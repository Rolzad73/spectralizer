/*************************************************************************
 * This file is part of spectralizer
 * github.con/univrsal/spectralizer
 * Copyright 2020 univrsal <universailp@web.de>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "bar_visualizer.hpp"
#include "../../source/visualizer_source.hpp"
#include "../../util/util.hpp"

namespace audio {

void bar_visualizer::draw_rectangle_bars()
{
    size_t i = 0, pos_x = 0, pos_y = 0;
    uint32_t height;
    for (; i < m_bars_left.size() - DEAD_BAR_OFFSET; i++) { /* Leave the four dead bars the end */
        auto val = m_bars_left[i] > 1.0 ? m_bars_left[i] : 1.0;
        height = UTIL_MAX(static_cast<uint32_t>(round(val)), 1);
        height = UTIL_MIN(height, m_cfg->bar_height);

        pos_x = i * (m_cfg->bar_width + m_cfg->bar_space);
        draw_bar(pos_x, pos_y, height, 0);
    }
}

void bar_visualizer::draw_stereo_rectangle_bars()
{
    size_t i = 0;
    float pos_x = 0, pos_y = 0;
    uint32_t height_l, height_r;
    uint32_t offset = m_cfg->stereo_space / 2;
    uint32_t center = m_cfg->cy / 2;

    for (; i < m_bars_left.size() - DEAD_BAR_OFFSET; i++) { /* Leave the four dead bars the end */
        double bar_left = (m_bars_left[i] > 1.0 ? m_bars_left[i] : 1.0);
        double bar_right = (m_bars_right[i] > 1.0 ? m_bars_right[i] : 1.0);

        height_l = UTIL_MAX(static_cast<uint32_t>(round(bar_left)), 1);
        height_l = UTIL_MIN(height_l, (center - offset));
        height_r = UTIL_MAX(static_cast<uint32_t>(round(bar_right)), 1);
        height_r = UTIL_MIN(height_r, (center - offset));

        pos_x = i * (m_cfg->bar_width + m_cfg->bar_space);

        /* Top */
        pos_y = center + offset;
        draw_bar(pos_x, -pos_y, height_l, 0);

        /* Bottom */
        pos_y = center - height_r - offset;
        draw_bar(pos_x, -pos_y, height_r, GS_FLIP_V);
    }
}

void bar_visualizer::draw_rounded_bars()
{
    size_t i = 0, pos_x = 0;
    uint32_t height;
    uint32_t vert_count = m_cfg->corner_points * 4;
    for (; i < m_bars_left.size() - DEAD_BAR_OFFSET; i++) { /* Leave the four dead bars the end */
        vert_count = 0;
        auto val = m_bars_left[i] > 1.0 ? m_bars_left[i] : 1.0;

        // The bar needs to be at least a square so the circle fits
        height = UTIL_MAX(static_cast<uint32_t>(round(val)), m_cfg->bar_width);
        height = UTIL_MIN(height, m_cfg->bar_height);

        pos_x = i * (m_cfg->bar_width + m_cfg->bar_space);
        auto verts = make_rounded_rectangle(height);
        gs_matrix_push();
        gs_load_vertexbuffer(verts);
        gs_matrix_translate3f(pos_x, (m_cfg->bar_height - height), 0);
        gs_draw(GS_TRISTRIP, 0, (m_cfg->corner_points + 1) * 8 + 20);
        gs_matrix_pop();
        gs_vertexbuffer_destroy(verts);
    }
}

void bar_visualizer::draw_stereo_rounded_bars()
{
    size_t i = 0, pos_x = 0;
    uint32_t height_l, height_r;
    int32_t offset = m_cfg->stereo_space / 2;
    uint32_t center = m_cfg->bar_height / 2 + offset;

    for (; i < m_bars_left.size() - DEAD_BAR_OFFSET; i++) { /* Leave the four dead bars the end */
        double bar_left = (m_bars_left[i] > 1.0 ? m_bars_left[i] : 1.0);
        double bar_right = (m_bars_right[i] > 1.0 ? m_bars_right[i] : 1.0);

        // The bar needs to be at least a square so the circle fits
        height_l = UTIL_MAX(static_cast<uint32_t>(round(bar_left)), m_cfg->bar_width);
        height_l = UTIL_MIN(height_l, (m_cfg->bar_height / 2));
        height_r = UTIL_MAX(static_cast<uint32_t>(round(bar_right)), m_cfg->bar_width);
        height_r = UTIL_MIN(height_r, (m_cfg->bar_height / 2));

        pos_x = i * (m_cfg->bar_width + m_cfg->bar_space);
        auto verts_left = make_rounded_rectangle(height_l);
        auto verts_right = make_rounded_rectangle(height_r);

        /* Top */
        gs_matrix_push();
        gs_load_vertexbuffer(verts_left);
        gs_matrix_translate3f(pos_x, (center - height_l) - offset, 0);
        gs_draw(GS_TRISTRIP, 0, (m_cfg->corner_points + 1) * 8 + 20);
        gs_matrix_pop();

        /* Bottom */
        gs_matrix_push();
        gs_load_vertexbuffer(verts_right);
        gs_matrix_translate3f(pos_x, center + offset, 0);
        gs_draw(GS_TRISTRIP, 0, (m_cfg->corner_points + 1) * 8 + 20);
        gs_matrix_pop();

        gs_vertexbuffer_destroy(verts_left);
        gs_vertexbuffer_destroy(verts_right);
    }
}

void bar_visualizer::draw_bar(float pos_x, float pos_y, uint32_t height, uint32_t flip)
{
    switch (m_cfg->paint) {
    case PM_SOLID:
    case PM_GRADIENT:
        effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
        tech = gs_effect_get_technique(effect, "Draw");
        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);
        gs_matrix_push();

        gs_matrix_translate3f(pos_x, pos_y + (m_cfg->cy - height), 0);
        tex = gs_texrender_get_texture(texture_render);
        gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), tex);
        gs_draw_sprite(tex, flip, m_cfg->bar_width, height);

        gs_matrix_pop();
        gs_technique_end_pass(tech);
        gs_technique_end(tech);
        break;
    case PM_RANGE:
        effect = obs_get_base_effect(OBS_EFFECT_SOLID);
        tech = gs_effect_get_technique(effect, "Solid");
        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);
        gs_matrix_push();

        struct vec4 cur_color;
        float factor = (float)height / m_cfg->bar_height;
        gs_matrix_translate3f(pos_x, pos_y + (m_cfg->bar_height - height), 0);
        cur_color.x = colorVal2.x * (1.0f - factor) + colorVal.x * factor;
        cur_color.y = colorVal2.y * (1.0f - factor) + colorVal.y * factor;
        cur_color.z = colorVal2.z * (1.0f - factor) + colorVal.z * factor;
        cur_color.w = colorVal2.w * (1.0f - factor) + colorVal.w * factor;
        gs_effect_set_vec4(color, &cur_color);
        gs_draw_sprite(nullptr, 0, m_cfg->bar_width, height);

        gs_matrix_pop();
        gs_technique_end_pass(tech);
        gs_technique_end(tech);
        break;
    }
}

bar_visualizer::bar_visualizer(source::config *cfg) : spectrum_visualizer(cfg) {}

void bar_visualizer::render()
{
    /* Just in case */
    if (m_bars_left.size() != m_cfg->detail + DEAD_BAR_OFFSET)
        m_bars_left.resize(m_cfg->detail + DEAD_BAR_OFFSET, 0.0);
    if (m_bars_right.size() != m_cfg->detail + DEAD_BAR_OFFSET)
        m_bars_right.resize(m_cfg->detail + DEAD_BAR_OFFSET, 0.0);

    if (m_cfg->stereo) {
        if (m_cfg->rounded_corners) {
            draw_stereo_rounded_bars();
        } else {
            draw_stereo_rectangle_bars();
        }
    } else {
        if (m_cfg->rounded_corners) {
            draw_rounded_bars();
        } else {
            draw_rectangle_bars();
        }
    }
}

void bar_visualizer::update()
{
    spectrum_visualizer::update();
    vec4_from_rgba(&colorVal, m_cfg->color);
    vec4_from_rgba(&colorVal2, m_cfg->color2);

    obs_enter_graphics();

    if (!texture_render) { texture_render = gs_texrender_create(GS_RGBA, GS_ZS_NONE); }
    else { gs_texrender_reset(texture_render); }

    if (gs_texrender_begin(texture_render, m_cfg->bar_width, m_cfg->bar_height)) {
        gs_ortho(0.0f, (float)m_cfg->bar_width, 0.0f, (float)m_cfg->bar_height, -100.0, 100.0);
        effect = obs_get_base_effect(OBS_EFFECT_SOLID);
        tech = gs_effect_get_technique(effect, "Solid");
        color = gs_effect_get_param_by_name(effect, "color");

        gs_technique_begin(tech);
        gs_technique_begin_pass(tech, 0);

        struct vec4 clear_color;
        vec4_zero(&clear_color);
        gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

        switch (m_cfg->paint) {
        case PM_SOLID:
            gs_effect_set_vec4(color, &colorVal);
            gs_draw_sprite(nullptr, 0, m_cfg->bar_width, m_cfg->bar_height);
            break;
        case PM_GRADIENT:
            struct vec4 cur_color;
            float factor;
            float i;
            for (i = 0.0f; i <= m_cfg->bar_height; i += 1.0f) {
                gs_matrix_push();
                factor = i / m_cfg->bar_height;
                gs_matrix_translate3f(0, (m_cfg->bar_height - i), 0);
                cur_color.x = colorVal2.x * (1.0f - factor) + colorVal.x * factor;
                cur_color.y = colorVal2.y * (1.0f - factor) + colorVal.y * factor;
                cur_color.z = colorVal2.z * (1.0f - factor) + colorVal.z * factor;
                cur_color.w = colorVal2.w * (1.0f - factor) + colorVal.w * factor;
                gs_effect_set_vec4(color, &cur_color);
                gs_draw_sprite(nullptr, 0, m_cfg->bar_width, 1);
                gs_matrix_pop();
            }
            break;
        case PM_RANGE:
            // can't set texture here as color changes with height of bar
            break;
        }

        gs_technique_end_pass(tech);
        gs_technique_end(tech);
        gs_texrender_end(texture_render);
    }
    obs_leave_graphics();
}

}
