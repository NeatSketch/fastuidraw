/*!
 * \file painter_packer.hpp
 * \brief file painter_packer.hpp
 *
 * Copyright 2016 by Intel.
 *
 * Contact: kevin.rogovin@intel.com
 *
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 *
 * \author Kevin Rogovin <kevin.rogovin@intel.com>
 *
 */


#pragma once

#include <vector>
#include <list>
#include <cstring>

#include <fastuidraw/util/reference_counted.hpp>
#include <fastuidraw/util/vecN.hpp>
#include <fastuidraw/util/matrix.hpp>
#include <fastuidraw/util/c_array.hpp>

#include <fastuidraw/text/glyph_atlas.hpp>
#include <fastuidraw/colorstop_atlas.hpp>
#include <fastuidraw/image.hpp>

#include <fastuidraw/painter/painter_shader_set.hpp>
#include <fastuidraw/painter/painter_brush.hpp>
#include <fastuidraw/painter/painter_enums.hpp>
#include <fastuidraw/painter/painter_attribute_data.hpp>
#include <fastuidraw/painter/painter_attribute_writer.hpp>
#include <fastuidraw/painter/backend/painter_draw.hpp>
#include <fastuidraw/painter/backend/painter_backend.hpp>
#include <fastuidraw/painter/backend/painter_header.hpp>

#include "painter_packer_data.hpp"

namespace fastuidraw
{
/*!\addtogroup PainterBackend
 * @{
 */

  /*!
   * \brief
   * A PainterPacker packs data created by a Painter
   * to be fed to a PainterBackend to draw.
   */
  class PainterPacker:public reference_counted<PainterPacker>::non_concurrent
  {
  public:
    enum
      {
        /*!
         * The total number of different query stats
         * supported. Sync this with the last enumeration
         * in PainterEnums::query_stats_t
         */
        num_stats = PainterEnums::num_layers + 1
      };

    /*!
     * \brief
     * A DataCallBack represents a functor call back
     * from any of the PainterPacker::draw_generic()
     * methods called whenever a header is added.
     */
    class DataCallBack:public reference_counted<DataCallBack>::non_concurrent
    {
    public:
      /*!
       * Ctor.
       */
      DataCallBack(void);

      ~DataCallBack();

      /*!
       * Returns true if this DataCallBack is already active on a
       * PainterPacker object.
       */
      bool
      active(void) const;

      /*!
       * To be implemented by a derived class to implement the call back
       * issues whenever a \ref PainterHeader value is added.
       * \param h handle to active PainterDraw
       * \param original_value header values written to PainterDraw::m_store
       * \param mapped_location sub-array into PainterDraw::m_store where header is written
       */
      virtual
      void
      header_added(const reference_counted_ptr<const PainterDraw> &h,
                   const PainterHeader &original_value,
                   c_array<generic_data> mapped_location) = 0;

    private:
      friend class PainterPacker;
      void *m_d;
    };

    /*!
     * Ctor.
     * \param pool pool with which to make a default brush; this brush
     *             is used when draw_generic() is called and the passed
     *             PainterData object lacks a brush value
     * \param backend handle to PainterBackend for the constructed PainterPacker
     */
    explicit
    PainterPacker(PainterPackedValuePool &pool,
                  vecN<unsigned int, num_stats> &stats,
                  reference_counted_ptr<PainterBackend> backend);

    virtual
    ~PainterPacker();

    /*!
     * Returns the active composite shader
     */
    PainterCompositeShader*
    composite_shader(void) const
    {
      return m_composite_shader;
    }

    /*!
     * Returns the active 3D API blending mode.
     */
    BlendMode
    composite_mode(void) const
    {
      return m_composite_mode;
    }

    /*!
     * Sets the active composite shader.
     * \param h composite shader to use for compositing.
     * \param blend_mode 3D API blend mode.
     */
    void
    composite_shader(PainterCompositeShader *h, BlendMode blend_mode)
    {
      m_composite_shader = h;
      m_composite_mode = blend_mode;
    }

    /*!
     * Returns the active blend shader
     */
    PainterBlendShader*
    blend_shader(void) const
    {
      return m_blend_shader;
    }

    /*!
     * Sets the active blend shader.
     * \param h blend shader to use for blending.
     */
    void
    blend_shader(PainterBlendShader* h)
    {
      m_blend_shader = h;
    }

    /*!
     * Add a \ref DataCallBack to this PainterPacker. A fixed DataCallBack
     * can only be active on one PainterPacker, but a single PainterPacker
     * can have multiple objects active on it. Callback objects are called
     * in REVERSE ordered there are added (thus the most recent callback
     * objects are called first).
     */
    void
    add_callback(const reference_counted_ptr<DataCallBack> &callback);

    /*!
     * Remove a \ref DataCallBack from this PainterPacker.
     */
    void
    remove_callback(const reference_counted_ptr<DataCallBack> &callback);

    /*!
     * Indicate to start drawing. Commands are buffered and not
     * set to the backend until end() or flush() is called.
     * All draw commands must be between a begin() / end() pair.
     * \param surface the \ref PainterBackend::Surface to which
     *                 to render content
     * \param clear_color_buffer if true, clear the color buffer
     *                           on the viewport of the surface.
     */
    void
    begin(const reference_counted_ptr<PainterBackend::Surface> &surface,
          bool clear_color_buffer);

    /*!
     * Indicate to end drawing. Commands are buffered and not
     * sent to the backend until end() is called.
     * All draw commands must be between a begin() / end() pair.
     */
    void
    end(void);

    /*!
     * Send all accumulated rendering commands to the GPU.
     */
    void
    flush(bool clear_z);

    /*!
     * Returns the PainterBackend::Surface to which the Painter
     * is drawing. If there is no active surface, then returns
     * a null reference.
     */
    const reference_counted_ptr<PainterBackend::Surface>&
    surface(void) const;

    /*!
     * Add a draw break to execute an action.
     * \param action action to execute on draw break
     */
    void
    draw_break(const reference_counted_ptr<const PainterDraw::Action> &action);

    /*!
     * Draw generic attribute data
     * \param shader shader with which to draw data
     * \param data data for how to draw
     * \param attrib_chunks attribute data to draw
     * \param index_chunks the i'th element is index data into attrib_chunks[i]
     * \param index_adjusts if non-empty, the i'th element is the value by which
     *                      to adjust all of index_chunks[i]; if empty the index
     *                      values are not adjusted.
     * \param z z-value z value placed into the header
     */
    void
    draw_generic(const reference_counted_ptr<PainterItemShader> &shader,
                 const PainterPackerData &data,
                 c_array<const c_array<const PainterAttribute> > attrib_chunks,
                 c_array<const c_array<const PainterIndex> > index_chunks,
                 c_array<const int> index_adjusts,
                 int z)
    {
      draw_generic(shader, data, attrib_chunks, index_chunks,
                   index_adjusts, c_array<const unsigned int>(),
                   z);
    }

    /*!
     * Draw generic attribute data
     * \param shader shader with which to draw data
     * \param data data for how to draw
     * \param attrib_chunks attribute data to draw
     * \param index_chunks the i'th element is index data into attrib_chunks[K]
     *                     where K = attrib_chunk_selector[i]
     * \param index_adjusts if non-empty, the i'th element is the value by which
     *                      to adjust all of index_chunks[i]; if empty the index
     *                      values are not adjusted.
     * \param attrib_chunk_selector selects which attribute chunk to use for
     *        each index chunk
     * \param z z-value z value placed into the header
     */
    void
    draw_generic(const reference_counted_ptr<PainterItemShader> &shader,
                 const PainterPackerData &data,
                 c_array<const c_array<const PainterAttribute> > attrib_chunks,
                 c_array<const c_array<const PainterIndex> > index_chunks,
                 c_array<const int> index_adjusts,
                 c_array<const unsigned int> attrib_chunk_selector,
                 int z);
    /*!
     * Draw generic attribute data
     * \param shader shader with which to draw data
     * \param data data for how to draw
     * \param src DrawWriter to use to write attribute and index data
     * \param z z-value z value placed into the header
     */
    void
    draw_generic(const reference_counted_ptr<PainterItemShader> &shader,
                 const PainterPackerData &data,
                 const PainterAttributeWriter &src,
                 int z);

    /*!
     * Returns the PainterBackend::PerformanceHints of the underlying
     * PainterBackend of this PainterPacker.
     */
    const PainterBackend::PerformanceHints&
    hints(void)
    {
      return m_backend->hints();
    }

    /*!
     * Returns the current accumulated draw the PainterPacker is on
     */
    unsigned int
    current_draw(void);

    /*!
     * Returns the current number of indices written into the
     * current draw.
     */
    unsigned int
    current_indices_written(void);

    /* The data behind a PainterShaderGroup is also defined privately
     * within PainterPacker implementation, so to implement the
     * PainterShaderGroup methods, we implement them here and have
     * the actual implementation call them.
     */
    static
    uint32_t
    composite_group(const PainterShaderGroup *md);

    static
    uint32_t
    blend_group(const PainterShaderGroup *md);

    static
    uint32_t
    item_group(const PainterShaderGroup *md);

    static
    uint32_t
    brush(const PainterShaderGroup *md);

    static
    BlendMode
    composite_mode(const PainterShaderGroup *md);

  private:
    class per_draw_command;
    class painter_state_location
    {
    public:
      uint32_t m_clipping_data_loc;
      uint32_t m_item_matrix_data_loc;
      uint32_t m_brush_shader_data_loc;
      uint32_t m_item_shader_data_loc;
      uint32_t m_composite_shader_data_loc;
      uint32_t m_blend_shader_data_loc;
    };

    class Workroom
    {
    public:
      std::vector<unsigned int> m_attribs_loaded;
    };

    void
    start_new_command(void);

    void
    upload_draw_state(const PainterPackerData &draw_state);

    unsigned int
    compute_room_needed_for_packing(const PainterPackerData &draw_state);

    template<typename T>
    unsigned int
    compute_room_needed_for_packing(const PainterData::value<T> &obj);

    template<typename T>
    void
    draw_generic_implement(const reference_counted_ptr<PainterItemShader> &shader,
                           const PainterPackerData &data,
                           const T &src,
                           int z);

    void
    flush_implement(void);

    reference_counted_ptr<PainterBackend> m_backend;
    PainterData::value<PainterBrush> m_default_brush;
    unsigned int m_header_size;

    PainterBlendShader *m_blend_shader;
    PainterCompositeShader *m_composite_shader;
    BlendMode m_composite_mode;
    painter_state_location m_painter_state_location;
    unsigned int m_number_commands;

    reference_counted_ptr<PainterBackend::Surface> m_surface;
    bool m_clear_color_buffer;
    bool m_begin_new_target;
    std::vector<per_draw_command> m_accumulated_draws;
    reference_counted_ptr<const Image> m_last_binded_image;

    Workroom m_work_room;
    vecN<unsigned int, num_stats> &m_stats;

    std::list<reference_counted_ptr<PainterPacker::DataCallBack> > m_callback_list;
  };
/*! @} */

}
