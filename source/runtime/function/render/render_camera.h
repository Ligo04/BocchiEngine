#pragma once
#include <UGM/UGM.hpp>
#include <mutex>
#include <vector>

namespace bocchi
{
    using namespace Ubpa;

    enum class RenderCameraType : int
    {
        kEditor,
        kMotor
    };

    class RenderCamera
    {
    public:
        RenderCameraType m_current_camera_type_ {RenderCameraType::kEditor};

        static const vecf3 m_x_, m_y_, m_z_;

        // transfomr
        transformf m_transform_ {};

        pointf3 m_position_ {0.0f, 0.0f, 0.0f};
        quatf m_rotation_ {quatf::identity()};
        quatf m_inv_rotation_ {quatf::identity()};
        float m_znear_ {1000.0f};
        float m_zfar_ {0.1f};
        vecf3 m_up_axis_ {m_z_};

        static constexpr float kMMinFov {10.0f};
        static constexpr float kMMaxFov {89.0f};
        static constexpr int   kMMainViewMatrixIndex {0};

        std::vector<transformf> m_view_matrices_ {transformf::eye()};

        void SetCurrentCameraType(RenderCameraType type);
        void SetMainViewMatrix(const transformf& view_matrix, RenderCameraType type = RenderCameraType::kEditor);

        void move(const vecf3& delta);
        void rotate(vecf2 delta);
        void zoom(float offset);
        void LookAt(const pointf3& position, const pointf3& target, const vecf3& up);

        void SetAspect(float aspect);
        void SetFoVx(float fovx) { m_fov_x_ = fovx; }

        [[nodiscard]] pointf3 Position() const { return m_position_; }
        quatf   Rotation() const { return m_rotation_; }

        vecf3 Forward() const { return (m_inv_rotation_*m_y_); }
        vecf3 Up() const { return (m_inv_rotation_*m_z_); }
        vecf3 Right() const { return (m_inv_rotation_*m_x_); }

        vecf2 GetFov() const { return vecf2 {m_fov_x_, m_fov_y_}; }

        transformf GetViewMatrix();

        transformf GetPersProjMatrix() const;

        transformf GetLookAtMatrix() const { return transformf::look_at(Position(), Position() + Forward(), Up()); }
        float      GetFovYDeprecated() const { return m_fov_y_; }

    protected:
        float m_aspect_ {0.0f};
        float m_fov_x_ {to_degree(89.0f)};
        float m_fov_y_ {0.0f};

        std::mutex m_view_matrix_mutex_;
    };

    inline const vecf3 RenderCamera::m_x_ = vecf3 {1.0f, 0.0f, 0.0f};
    inline const vecf3 RenderCamera::m_y_ = vecf3 {0.0f, 1.0f, 0.0f};
    inline const vecf3 RenderCamera::m_z_ = vecf3 {0.0f, 0.0f, 1.0f};
} // namespace bocchi