#include "file_system.h"

#include <fstream>
#include <sstream>

#include "runtime/core/base/macro.h"
#include "runtime/core/base/string_utils.h"

namespace bocchi
{
    Blob::Blob(void* data, size_t size) : m_data_(data), m_size_(size) {}

    Blob::~Blob()
    {
        if (m_data_)
        {
            free(m_data_);
            m_data_ = nullptr;
        }

        m_size_ = 0;
    }

    const void* Blob::data() const { return m_data_; }

    size_t Blob::size() const { return m_size_; }

    bool NativeFileSystem::FolderExists(const std::filesystem::path& name)
    {
        return std::filesystem::exists(name) && std::filesystem::is_directory(name);
    }

    bool NativeFileSystem::FileExists(const std::filesystem::path& name)
    {
        return std::filesystem::exists(name) && std::filesystem::is_regular_file(name);
    }

    std::shared_ptr<IBlob> NativeFileSystem::ReadFile(const std::filesystem::path& name)
    {
        // TODO: better error reporting

        std::ifstream file(name, std::ios::binary);

        if (!file.is_open())
        {
            // file does not exist or is locked
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        uint64_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        {
            // file larger than size_t
            assert(false);
            return nullptr;
        }

        char* data = static_cast<char*>(malloc(size));

        if (data == nullptr)
        {
            // out of memory
            assert(false);
            return nullptr;
        }

        file.read(data, size);

        if (!file.good())
        {
            // reading error
            assert(false);
            return nullptr;
        }

        return std::make_shared<Blob>(data, size);
    }

    bool NativeFileSystem::WriteFile(const std::filesystem::path& name, const void* data, size_t size)
    {
        // TODO: better error reporting

        std::ofstream file(name, std::ios::binary);

        if (!file.is_open())
        {
            // file does not exist or is locked
            return false;
        }

        if (size > 0)
        {
            file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
        }

        if (!file.good())
        {
            // writing error
            return false;
        }

        return true;
    }

    static int EnumerateNativeFiles(const char* pattern, bool directories, EnumerateCallbackT callback)
    {
#ifdef WIN32

        WIN32_FIND_DATAA find_data;
        HANDLE           h_find = FindFirstFileA(pattern, &find_data);

        if (h_find == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                return 0;

            return status::kFailed;
        }

        int num_entries = 0;

        do
        {
            bool is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            bool is_dot       = strcmp(find_data.cFileName, ".") == 0;
            bool is_dot_dot    = strcmp(find_data.cFileName, "..") == 0;

            if ((is_directory == directories) && !is_dot && !is_dot_dot)
            {
                callback(find_data.cFileName);
                ++num_entries;
            }
        } while (FindNextFileA(h_find, &find_data) != 0);

        FindClose(h_find);

        return num_entries;

#else // WIN32

        glob64_t glob_matches;
        int      globResult = glob64(pattern, 0 /*flags*/, nullptr /*errfunc*/, &glob_matches);

        if (globResult == 0)
        {
            int numEntries = 0;

            for (int i = 0; i < glob_matches.gl_pathc; ++i)
            {
                const char*                      globentry = (glob_matches.gl_pathv)[i];
                std::error_code                  ec, ec2;
                std::filesystem::directory_entry entry(globentry, ec);
                if (!ec)
                {
                    if (directories == entry.is_directory(ec2) && !ec2)
                    {
                        callback(entry.path().filename().native());
                        ++numEntries;
                    }
                }
            }
            globfree64(&glob_matches);

            return numEntries;
        }

        if (globResult == GLOB_NOMATCH)
            return 0;

        return status::Failed;

#endif // WIN32
    }

    int NativeFileSystem::EnumerateFiles(const std::filesystem::path&    path,
                                         const std::vector<std::string>& extensions,
                                         EnumerateCallbackT              callback,
                                         bool                            allow_duplicates)
    {
        (void)allow_duplicates;

        if (extensions.empty())
        {
            std::string pattern = (path / "*").generic_string();
            return EnumerateNativeFiles(pattern.c_str(), false, callback);
        }

        int num_entries = 0;
        for (const auto& ext : extensions)
        {
            std::string pattern = (path / ("*" + ext)).generic_string();
            int         result  = EnumerateNativeFiles(pattern.c_str(), false, callback);

            if (result < 0)
                return result;

            num_entries += result;
        }

        return num_entries;
    }

    int NativeFileSystem::EnumerateDirectories(const std::filesystem::path& path,
                                               EnumerateCallbackT           callback,
                                               bool                         allow_duplicates)
    {
        (void)allow_duplicates;

        std::string pattern = (path / "*").generic_string();
        return EnumerateNativeFiles(pattern.c_str(), true, callback);
    }

    RelativeFileSystem::RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& base_path) :
        m_underlying_fs_(std::move(fs)), m_base_path_(base_path.lexically_normal())
    {}

    bool RelativeFileSystem::FolderExists(const std::filesystem::path& name)
    {
        return m_underlying_fs_->FolderExists(m_base_path_ / name.relative_path());
    }

    bool RelativeFileSystem::FileExists(const std::filesystem::path& name)
    {
        return m_underlying_fs_->FileExists(m_base_path_ / name.relative_path());
    }

    std::shared_ptr<IBlob> RelativeFileSystem::ReadFile(const std::filesystem::path& name)
    {
        return m_underlying_fs_->ReadFile(m_base_path_ / name.relative_path());
    }

    bool RelativeFileSystem::WriteFile(const std::filesystem::path& name, const void* data, size_t size)
    {
        return m_underlying_fs_->WriteFile(m_base_path_ / name.relative_path(), data, size);
    }

    int RelativeFileSystem::EnumerateFiles(const std::filesystem::path&    path,
                                           const std::vector<std::string>& extensions,
                                           EnumerateCallbackT              callback,
                                           bool                            allow_duplicates)
    {
        return m_underlying_fs_->EnumerateFiles(
            m_base_path_ / path.relative_path(), extensions, callback, allow_duplicates);
    }

    int RelativeFileSystem::EnumerateDirectories(const std::filesystem::path& path,
                                                 EnumerateCallbackT           callback,
                                                 bool                         allow_duplicates)
    {
        return m_underlying_fs_->EnumerateDirectories(m_base_path_ / path.relative_path(), callback, allow_duplicates);
    }

    void RootFileSystem::mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs)
    {
        if (FindMountPoint(path, nullptr, nullptr))
        {
            std::stringstream ss;
            ss << "Cannot mount a filesystem at " << path.c_str() << ": there is another FS that includes this path";
            LOG_ERROR(ss.str());
            return;
        }

        m_mount_points_.push_back(std::make_pair(path.lexically_normal().generic_string(), fs));
    }

    void RootFileSystem::mount(const std::filesystem::path& path, const std::filesystem::path& native_path)
    {
        mount(path, std::make_shared<RelativeFileSystem>(std::make_shared<NativeFileSystem>(), native_path));
    }

    bool RootFileSystem::unmount(const std::filesystem::path& path)
    {
        std::string spath = path.lexically_normal().generic_string();

        for (size_t index = 0; index < m_mount_points_.size(); index++)
        {
            if (m_mount_points_[index].first == spath)
            {
                m_mount_points_.erase(m_mount_points_.begin() + index);
                return true;
            }
        }

        return false;
    }

    bool RootFileSystem::FindMountPoint(const std::filesystem::path& path,
                                        std::filesystem::path*       p_relative_path,
                                        IFileSystem**                pp_fs)
    {
        std::string spath = path.lexically_normal().generic_string();

        for (auto it : m_mount_points_)
        {
            if (spath.find(it.first, 0) == 0 &&
                ((spath.length() == it.first.length()) || (spath[it.first.length()] == '/')))
            {
                if (p_relative_path)
                {
                    std::string relative = spath.substr(it.first.size() + 1);
                    *p_relative_path     = relative;
                }

                if (pp_fs)
                {
                    *pp_fs = it.second.get();
                }

                return true;
            }
        }

        return false;
    }

    bool RootFileSystem::FolderExists(const std::filesystem::path& name)
    {
        std::filesystem::path relative_path;
        IFileSystem*          fs = nullptr;

        if (FindMountPoint(name, &relative_path, &fs))
        {
            return fs->FolderExists(relative_path);
        }

        return false;
    }

    bool RootFileSystem::FileExists(const std::filesystem::path& name)
    {
        std::filesystem::path relative_path;
        IFileSystem*          fs = nullptr;

        if (FindMountPoint(name, &relative_path, &fs))
        {
            return fs->FileExists(relative_path);
        }

        return false;
    }

    std::shared_ptr<IBlob> RootFileSystem::ReadFile(const std::filesystem::path& name)
    {
        std::filesystem::path relative_path;
        IFileSystem*          fs = nullptr;

        if (FindMountPoint(name, &relative_path, &fs))
        {
            return fs->ReadFile(relative_path);
        }

        return nullptr;
    }

    bool RootFileSystem::WriteFile(const std::filesystem::path& name, const void* data, size_t size)
    {
        std::filesystem::path relative_path;
        IFileSystem*          fs = nullptr;

        if (FindMountPoint(name, &relative_path, &fs))
        {
            return fs->WriteFile(relative_path, data, size);
        }

        return false;
    }

    int RootFileSystem::EnumerateFiles(const std::filesystem::path&    path,
                                       const std::vector<std::string>& extensions,
                                       EnumerateCallbackT              callback,
                                       bool                            allow_duplicates)
    {
        std::filesystem::path relative_path;
        IFileSystem*          fs = nullptr;

        if (FindMountPoint(path, &relative_path, &fs))
        {
            return fs->EnumerateFiles(relative_path, extensions, callback, allow_duplicates);
        }

        return status::kPathNotFound;
    }

    int RootFileSystem::EnumerateDirectories(const std::filesystem::path& path,
                                             EnumerateCallbackT           callback,
                                             bool                         allow_duplicates)
    {
        std::filesystem::path relative_path;
        IFileSystem*          fs = nullptr;

        if (FindMountPoint(path, &relative_path, &fs))
        {
            return fs->EnumerateDirectories(relative_path, callback, allow_duplicates);
        }

        return status::kPathNotFound;
    }

    static void AppendPatternToRegex(const std::string& pattern, std::stringstream& regex)
    {
        for (char c : pattern)
        {
            switch (c)
            {
                case '?':
                    regex << "[^/]?";
                    break;
                case '*':
                    regex << "[^/]+";
                    break;
                case '.':
                    regex << "\\.";
                    break;
                default:
                    regex << c;
            }
        }
    }

    std::string GetFileSearchRegex(const std::filesystem::path& path, const std::vector<std::string>& extensions)
    {
        std::filesystem::path normalized_path     = path.lexically_normal();
        std::string           normalized_path_str = normalized_path.generic_string();

        std::stringstream regex;
        AppendPatternToRegex(normalized_path_str, regex);
        if (!string_utils::ends_with(normalized_path_str, "/") && !normalized_path.empty())
            regex << '/';
        regex << "[^/]+";

        if (!extensions.empty())
        {
            regex << '(';
            bool first = true;
            for (const auto& ext : extensions)
            {
                if (!first)
                    regex << '|';
                AppendPatternToRegex(ext, regex);
                first = false;
            }
            regex << ')';
        }

        return regex.str();
    }
} // namespace bocchi
