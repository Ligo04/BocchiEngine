#include "render_camera.h"

namespace bocchi
{
    void RenderCamera::SetCurrentCameraType(RenderCameraType type)
    {
        std::lock_guard lock_guard(m_view_matrix_mutex);
        m_current_camera_type = type;
    }

    void RenderCamera::SetMainViewMatrix(const transformf& view_matrix, RenderCameraType type)
    {
        std::lock_guard lock_guard(m_view_matrix_mutex);
        m_current_camera_type                        = type;
        m_view_matrices[m_m_main_view_matrix_index] = view_matrix;
    }

    void RenderCamera::move(const vecf3& delta) { m_position += delta; }

    void RenderCamera::rotate(vecf2 delta)
    {
        delta = vecf2(to_radian(to_degree(delta[0])), to_radian(to_degree(delta[1])));

        // limit pitch
        if (const float dot = m_up_axis.dot(Forward());
            (dot < -0.99f && delta[0] > 0.0f) || // angle nearing 180 degrees
            (dot > 0.99f && delta[0] < 0.0f))    // angle nearing 0 degrees
        {
            delta[0] = 0.0f;
        }

        // pitch is relative to current sideways rotation
        // yaw happens independently
        // this prevents roll
        const quatf pitch(to_radian(delta[0]), m_x);
        const quatf yaw(to_radian(delta[1]), m_z);

        m_rotation = pitch * m_rotation * yaw;

        m_inv_rotation = m_rotation.inverse();
    }

    void RenderCamera::zoom(const float offset)
    {
        m_fov_x = std::clamp(m_fov_x + offset, m_m_min_fov, m_m_max_fov);
    }

    void RenderCamera::LookAt(const pointf3& position, const pointf3& target, const vecf3& up)
    {
        m_position = position;

        // model rotation
        // maps vectors to camera space (x,y,z)
        const vecf3 forward = (target - position).normalize();
        m_rotation         = quatf(forward, m_y);

        // corrent the up vector
        const vecf3 right  = forward.cross(up.normalize()).normalize();
        const vecf3 ort_up = right.cross(forward);

        const auto up_rotation = quatf(m_rotation * ort_up, m_z);

        m_rotation = up_rotation * m_rotation;

        // inverse of the model rotation
        // maps camera space vector to model vectors
        m_inv_rotation = m_rotation.inverse();
    }

    void RenderCamera::SetAspect(const float aspect)
    {
        m_aspect = aspect;
        m_fov_y  = to_radian(atanf(tan(to_radian(to_degree(m_fov_x) * 0.5f)) / m_aspect) * 2.0f);
    }

    transformf RenderCamera::GetViewMatrix()
    {
        std::lock_guard lock_guard(m_view_matrix_mutex);
        transformf      view_matrix = transformf::eye();

        switch (m_current_camera_type)
        {
            case RenderCameraType::Editor:
                view_matrix = transformf::look_at(Position(), Position() + Forward(), Up());
                break;

            case RenderCameraType::Motor:
                view_matrix = m_view_matrices[m_m_main_view_matrix_index];
                break;

            default:
                break;
        }
        return view_matrix;
    }

    transformf RenderCamera::GetPersProjMatrix() const
    {
        const auto fix_mat = transformf(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
        return fix_mat * transformf::perspective(to_radian(to_degree(m_fov_y)), m_aspect, m_znear, m_zfar);
    }
} // namespace bocchi
