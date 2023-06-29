#pragma once

#include <filesystem>
#include <functional>

namespace bocchi
{
    namespace status
    {
        constexpr int kOk             = 0;
        constexpr int kFailed         = -1;
        constexpr int kPathNotFound   = -2;
        constexpr int kNotImplemented = -3;
    } // namespace status

    typedef const std::function<void(std::string_view)>& EnumerateCallbackT;

    // A blob is a package for untyped data, typically read from a file.
    class IBlob
    {
    public:
        virtual ~IBlob()                               = default;
        [[nodiscard]] virtual const void* data() const = 0;
        [[nodiscard]] virtual size_t      size() const = 0;

        static bool IsEmpty(IBlob const& blob) { return blob.data() != nullptr && blob.size() != 0; }
    };

    // Specific blob implementation that owns the data and frees it when deleted.
    class Blob : public IBlob
    {
    public:
        void*  m_data_;
        size_t m_size_;

    public:
        Blob(void* data, size_t size);
        ~Blob() override;

        [[nodiscard]] const void* data() const override;
        [[nodiscard]] size_t      size() const override;
    };

    // Basic interface for the virtual file system
    class IFileSystem
    {
    public:
        virtual ~IFileSystem() = default;

        // Test if a folder exists.
        virtual bool FolderExists(const std::filesystem::path& name) = 0;

        // Test if a file exists.
        virtual bool FileExists(const std::filesystem::path& name) = 0;

        // Read the entire file.
        // Returns nullptr if the file cannot be read.
        virtual std::shared_ptr<IBlob> ReadFile(const std::filesystem::path& name) = 0;

        // Write the entire file.
        // Returns false if the file cannot be written.
        virtual bool WriteFile(const std::filesystem::path& name, const void* data, size_t size) = 0;

        // Search for files with any of the provided 'extensions' in 'path'.
        // Extensions should not include any wildcard characters.
        // Returns the number of files found, or a negative number on errors - see donut::vfs::status.
        // The file names, relative to the 'path', are passed to 'callback' in no particular order.
        virtual int EnumerateFiles(const std::filesystem::path&    path,
                                   const std::vector<std::string>& extensions,
                                   EnumerateCallbackT              callback,
                                   bool                            allow_duplicates = false) = 0;

        // Search for directories in 'path'.
        // Returns the number of directories found, or a negative number on errors - see donut::vfs::status.
        // The directory names, relative to the 'path', are passed to 'callback' in no particular order.
        virtual int EnumerateDirectories(const std::filesystem::path& path,
                                         EnumerateCallbackT           callback,
                                         bool                         allow_duplicates = false) = 0;
    };

    // An implementation of virtual file system that directly maps to the OS files.
    class NativeFileSystem : public IFileSystem
    {
    public:
        bool                   FolderExists(const std::filesystem::path& name) override;
        bool                   FileExists(const std::filesystem::path& name) override;
        std::shared_ptr<IBlob> ReadFile(const std::filesystem::path& name) override;
        bool                   WriteFile(const std::filesystem::path& name, const void* data, size_t size) override;
        int                    EnumerateFiles(const std::filesystem::path&    path,
                                              const std::vector<std::string>& extensions,
                                              EnumerateCallbackT              callback,
                                              bool                            allow_duplicates = false) override;
        int                    EnumerateDirectories(const std::filesystem::path& path,
                                                    EnumerateCallbackT           callback,
                                                    bool                         allow_duplicates = false) override;
    };

    // A layer that represents some path in the underlying file system as an entire FS.
    // Effectively, just prepends the provided base path to every file name
    // and passes the requests to the underlying FS.
    class RelativeFileSystem : public IFileSystem
    {
    private:
        std::shared_ptr<IFileSystem> m_underlying_fs_;
        std::filesystem::path        m_base_path_;

    public:
        RelativeFileSystem(std::shared_ptr<IFileSystem> fs, const std::filesystem::path& base_path);

        [[nodiscard]] std::filesystem::path const& GetBasePath() const { return m_base_path_; }

        bool                   FolderExists(const std::filesystem::path& name) override;
        bool                   FileExists(const std::filesystem::path& name) override;
        std::shared_ptr<IBlob> ReadFile(const std::filesystem::path& name) override;
        bool                   WriteFile(const std::filesystem::path& name, const void* data, size_t size) override;
        int                    EnumerateFiles(const std::filesystem::path&    path,
                                              const std::vector<std::string>& extensions,
                                              EnumerateCallbackT              callback,
                                              bool                            allow_duplicates = false) override;
        int                    EnumerateDirectories(const std::filesystem::path& path,
                                                    EnumerateCallbackT           callback,
                                                    bool                         allow_duplicates = false) override;
    };

    // A virtual file system that allows mounting, or attaching, other VFS objects to paths.
    // Does not have any file systems by default, all of them must be mounted first.
    class RootFileSystem : public IFileSystem
    {
    private:
        std::vector<std::pair<std::string, std::shared_ptr<IFileSystem>>> m_mount_points_;

        bool
        FindMountPoint(const std::filesystem::path& path, std::filesystem::path* p_relative_path, IFileSystem** pp_fs);

    public:
        void mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs);
        void mount(const std::filesystem::path& path, const std::filesystem::path& native_path);
        bool unmount(const std::filesystem::path& path);

        bool                   FolderExists(const std::filesystem::path& name) override;
        bool                   FileExists(const std::filesystem::path& name) override;
        std::shared_ptr<IBlob> ReadFile(const std::filesystem::path& name) override;
        bool                   WriteFile(const std::filesystem::path& name, const void* data, size_t size) override;
        int                    EnumerateFiles(const std::filesystem::path&    path,
                                              const std::vector<std::string>& extensions,
                                              EnumerateCallbackT              callback,
                                              bool                            allow_duplicates = false) override;

        int                    EnumerateDirectories(const std::filesystem::path& path,
                                                    EnumerateCallbackT           callback,
                                                    bool                         allow_duplicates = false) override;
    };

    std::string GetFileSearchRegex(const std::filesystem::path& path, const std::vector<std::string>& extensions);
} // namespace bocchi
