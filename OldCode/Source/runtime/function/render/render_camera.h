#pragma once
#include <UGM/UGM.hpp>
#include <mutex>
#include <vector>

namespace bocchi
{
    using namespace Ubpa;

    enum class RenderCameraType : int
    {
        Editor,
        Motor
    };

    class RenderCamera
    {
    public:
        RenderCameraType m_current_camera_type {RenderCameraType::Editor};

        static const vecf3 m_x, m_y, m_z;

        pointf3 m_position {0.0f, 0.0f, 0.0f};
        quatf m_rotation {quatf::identity()};
        quatf m_inv_rotation {quatf::identity()};
        float   m_znear {0.1f};
        float   m_zfar {1000.0f};
        vecf3 m_up_axis {m_z};

        static constexpr float m_m_min_fov {10.0f};
        static constexpr float m_m_max_fov {89.0f};
        static constexpr int   m_m_main_view_matrix_index {0};

        std::vector<transformf> m_view_matrices {transformf::eye()};

        void SetCurrentCameraType(RenderCameraType type);
        void SetMainViewMatrix(const transformf& view_matrix, RenderCameraType type = RenderCameraType::Editor);

        void move(const vecf3& delta);
        void rotate(vecf2 delta);
        void zoom(float offset);
        void LookAt(const pointf3& position, const pointf3& target, const vecf3& up);

        void SetAspect(float aspect);
        void SetFoVx(const float fovx) { m_fov_x = fovx; }

        [[nodiscard]] pointf3 Position() const { return m_position; }
        [[nodiscard]] quatf   Rotation() const { return m_rotation; }

        [[nodiscard]] vecf3 Forward() const { return (m_inv_rotation * m_y); }
        [[nodiscard]] vecf3 Up() const { return (m_inv_rotation * m_z); }
        [[nodiscard]] vecf3 Right() const { return (m_inv_rotation * m_x); }

        [[nodiscard]] vecf2 GetFov() const { return vecf2 {m_fov_x, m_fov_y}; }

        transformf GetViewMatrix();

        [[nodiscard]] transformf GetPersProjMatrix() const;

        [[nodiscard]] transformf GetLookAtMatrix() const
        {
            return transformf::look_at(Position(), Position() + Forward(), Up());
        }
        [[nodiscard]] float GetFovYDeprecated() const { return m_fov_y; }

    protected:
        float m_aspect {0.0f};
        float m_fov_x {to_degree(89.0f)};
        float m_fov_y {0.0f};

        std::mutex m_view_matrix_mutex;
    };

    inline const vecf3 RenderCamera::m_x = vecf3 {1.0f, 0.0f, 0.0f};
    inline const vecf3 RenderCamera::m_y = vecf3 {0.0f, 1.0f, 0.0f};
    inline const vecf3 RenderCamera::m_z = vecf3 {0.0f, 0.0f, 1.0f};
} // namespace bocchi