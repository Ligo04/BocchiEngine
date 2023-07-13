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
        m_view_matrices_[m_m_main_view_matrix_index_] = view_matrix;
    }

    void RenderCamera::move(const vecf3& delta) { m_position_ += delta; }

    void RenderCamera::rotate(vecf2 delta)
    {
        delta = vecf2(to_radian(to_degree(delta[0])), to_radian(to_degree(delta[1])));

        // limit pitch
        if ( const float dot = m_up_axis_.dot(Forward()) ;
            (dot < -0.99f && delta[0] > 0.0f) || // angle nearing 180 degrees
            (dot > 0.99f && delta[0] < 0.0f))                                                   // angle nearing 0 degrees
        {
            delta[0] = 0.0f;
        }

        // pitch is relative to current sideways rotation
        // yaw happens independently
        // this prevents roll
        const quatf pitch(to_radian(delta[0]), m_x_);
        const quatf yaw(to_radian(delta[1]), m_z_);

        m_rotation_ = pitch * m_rotation_ * yaw;

        m_inv_rotation_ = m_rotation_.inverse();
    }

    void RenderCamera::zoom(const float offset) { m_fov_x_ = std::clamp(m_fov_x_ + offset, m_m_min_fov_, m_m_max_fov_); }

    void RenderCamera::LookAt(const pointf3& position, const pointf3& target, const vecf3& up)
    {
        m_position_ = position;

        // model rotation
        // maps vectors to camera space (x,y,z)
        const vecf3 forward = (target - position).normalize();
        m_rotation_         = quatf(forward, m_y_);

        // corrent the up vector
        const vecf3 right  = forward.cross(up.normalize()).normalize();
        const vecf3 ort_up = right.cross(forward);

        const auto up_rotation = quatf(m_rotation_ * ort_up, m_z_);

        m_rotation_ = up_rotation * m_rotation_;

        // inverse of the model rotation
        // maps camera space vector to model vectors
        m_inv_rotation_ = m_rotation_.inverse();
    }

    void RenderCamera::SetAspect(const float aspect)
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
            case RenderCameraType::Editor:
                view_matrix = transformf::look_at(Position(), Position() + Forward(), Up());
                break;

            case RenderCameraType::Motor:
                view_matrix = m_view_matrices_[m_m_main_view_matrix_index_];
                break;

            default:
                break;
        }
        return view_matrix;
    }

    transformf RenderCamera::GetPersProjMatrix() const
    {
        const auto fix_mat = transformf(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        return fix_mat * transformf::perspective(to_radian(to_degree(m_fov_y_)), m_aspect_, m_znear_, m_zfar_);
    }
} // namespace bocchi
