#include "render_camera.h"

namespace bocchi
{
    void RenderCamera::SetCurrentCameraType(RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex_);
        m_current_camera_type_ = type;
    }

    void RenderCamera::SetMainViewMatrix(const transformf& view_matrix, RenderCameraType type)
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex_);
        m_current_camera_type_                  = type;
        m_view_matrices_[kMMainViewMatrixIndex] = view_matrix;
    }

    void RenderCamera::move(const vecf3& delta) { m_position_ += delta; }

    void RenderCamera::rotate(vecf2 delta)
    {
        delta = vecf2(to_radian(to_degree(delta[0])), to_radian(to_degree(delta[1])));

        // limit pitch
        float dot = m_up_axis_.dot(Forward());
        if ((dot < -0.99f && delta[0] > 0.0f) || // angle nearing 180 degrees
            (dot > 0.99f && delta[0] < 0.0f))    // angle nearing 0 degrees
        {
            delta[0] = 0.0f;
        }

        // pitch is relative to current sideways rotation
        // yaw happens independently
        // this prevents roll
        quatf pitch(to_radian(delta[0]), m_x_);
        quatf yaw(to_radian(delta[1]), m_z_);

        m_rotation_ = pitch * m_rotation_ * yaw;

        m_inv_rotation_ = m_rotation_.inverse();
    }

    void RenderCamera::zoom(float offset) { m_fov_x_ = std::clamp(m_fov_x_ + offset, kMMinFov, kMMaxFov); }

    void RenderCamera::LookAt(const pointf3& position, const pointf3& target, const vecf3& up)
    {
        m_position_ = position;

        // model rotation
        // maps vectors to camera space (x,y,z)
        vecf3 forward = (target - position).normalize();
        m_rotation_   = quatf(forward, m_y_);

        // corrent the up vector
        vecf3 right  = forward.cross(up.normalize()).normalize();
        vecf3 ort_up = right.cross(forward);

        quatf up_rotation = quatf(m_rotation_ * ort_up, m_z_);

        m_rotation_ = up_rotation * m_rotation_;

        // inverse of the model rotation
        // maps camera space vector to model vectors
        m_inv_rotation_ = m_rotation_.inverse();
    }

    void RenderCamera::SetAspect(float aspect)
    {
        m_aspect_ = aspect;
        m_fov_y_  = to_radian(atanf(tan(to_radian(to_degree(m_fov_x_) * 0.5f)) / m_aspect_) * 2.0f);
    }

    transformf RenderCamera::GetViewMatrix()
    {
        std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex_);
        transformf                  view_matrix = transformf::eye();

        switch (m_current_camera_type_)
        {
            case RenderCameraType::kEditor:
                view_matrix = transformf::look_at(Position(), Position() + Forward(), Up());
                break;

            case RenderCameraType::kMotor:
                view_matrix = m_view_matrices_[kMMainViewMatrixIndex];
                break;

            default:
                break;
        }
        return view_matrix;
    }
    transformf RenderCamera::GetPersProjMatrix() const
    {
        transformf fix_mat = transformf(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        return transformf::perspective(to_radian(to_degree(m_fov_y_)), m_aspect_, m_znear_, m_zfar_);
    }
} // namespace bocchi
