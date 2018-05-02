/*!
 * \file arc_tessellated_path.hpp
 * \brief file arc_tessellated_path.hpp
 *
 * Copyright 2018 by Intel.
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


#include <fastuidraw/util/fastuidraw_memory.hpp>
#include <fastuidraw/util/vecN.hpp>
#include <fastuidraw/util/c_array.hpp>
#include <fastuidraw/util/reference_counted.hpp>
#include <fastuidraw/painter/stroked_caps_joins.hpp>

namespace fastuidraw  {

///@cond
class Path;
class ArcStrokedPath;
///@endcond

/*!\addtogroup Paths
 * @{
 */

/*!
 * \brief
 * An ArcTessellatedPath represents the tessellation of a Path
 * into line segments and arcs.
 *
 * A single contour of a ArcTessellatedPath is constructed from
 * a single \ref PathContour of the source \ref Path. Each
 * edge of a contour of a ArcTessellatedPath is contructed from
 * a single \ref PathContour::interpolator_base of the source \ref
 * PathContour. The ordering of the contours of a
 * ArcTessellatedPath is the same ordering as the source
 * \ref PathContour objects of the source \ref Path. Also,
 * the ordering of edges within a contour is the same ordering
 * as the \ref PathContour::interpolator_base objects of
 * the source \ref PathContour. In particular, for each contour
 * of a ArcTessellatedPath, the closing edge is the last edge.
 */
class ArcTessellatedPath:
    public reference_counted<ArcTessellatedPath>::non_concurrent
{
public:
  /*!
   */
  enum segment_type_t
    {
      arc_segment,
      line_segment,
    };

  /*!
   * \brief
   * A TessellationParams stores how finely to tessellate
   * the curves of a path.
   */
  class TessellationParams
  {
  public:
    /*!
     * Ctor, initializes values.
     */
    TessellationParams(void):
      m_threshhold(1.0f),
      m_max_segments(32)
    {}

    /*!
     * Non-equal comparison operator.
     * \param rhs value to which to compare against
     */
    bool
    operator!=(const TessellationParams &rhs) const
    {
      return m_threshhold != rhs.m_threshhold
        || m_max_segments != rhs.m_max_segments;
    }

    /*!
     * Provided as a conveniance. Equivalent to
     * \code
     * m_threshhold_type = tp;
     * \endcode
     * \param p value to which to assign to \ref m_threshhold
     */
    TessellationParams&
    threshhold(float p)
    {
      m_threshhold = p;
      return *this;
    }

    /*!
     * Set the value of \ref m_max_segments.
     * \param v value to which to assign to \ref m_max_segments
     */
    TessellationParams&
    max_segments(unsigned int v)
    {
      m_max_segments = v;
      return *this;
    }

    /*!
     * Meaning depends on \ref m_threshhold_type.
     * Default value is 1.0.
     */
    float m_threshhold;

    /*!
     * Maximum number of segments to tessellate each
     * PathContour::interpolator_base from each
     * PathContour of a Path. Default value is 32.
     */
    unsigned int m_max_segments;
  };

  /*!
   * \brief
   * Represents segment of an arc-tessellated path.
   */
  class segment
  {
  public:
    /*!
     * Specifies the segment type which in turn determines the
     * meaning of \ref m_p, \ref m_data and \ref m_radius
     */
    enum segment_type_t m_type;

    /*!
     * If \ref m_type is \ref line_segment, then gives the
     * start position of the segment. If \ref m_type is \ref
     * arc_segment, gives the center of the arc.
     */
    vec2 m_p;

    /*!
     * If \ref m_type is \ref line_segment, then gives the
     * the position where the segment ends. If \ref m_type
     * is \ref arc_segment, gives the start and end angles
     * of the arc.
     */
    vec2 m_data;

    /*!
     * Only valid if \ref m_type is \ref arc_segment; gives
     * the radius of the arc.
     */
    float m_radius;

    /*!
     * Gives the length of the segment.
     */
    float m_length;

    /*!
     * Gives the distance of the start of the segment from
     * the start of the edge (i.e PathContour::interpolator_base).
     */
    float m_distance_from_edge_start;

    /*!
     * Gives the distance of the start of segment to the
     * start of the -contour-.
     */
    float m_distance_from_contour_start;

    /*!
     * Gives the length of the edge (i.e.
     * PathContour::interpolator_base) on which the
     * segment lies. This value is the same for all
     * segments along a fixed edge.
     */
    float m_edge_length;

    /*!
     * Gives the length of the contour open on which
     * the segment lies. This value is the same for all
     * segments along a fixed contour.
     */
    float m_open_contour_length;

    /*!
     * Gives the length of the contour closed on which
     * the segment lies. This value is the same for all
     * segments along a fixed contour.
     */
    float m_closed_contour_length;
  };

  /*!
   * Ctor. Construct a TessellatedPath from a Path
   * \param input source path to tessellate
   * \param P parameters on how to tessellate the source Path
   */
  ArcTessellatedPath(const Path &input, TessellationParams P);

  ~ArcTessellatedPath();

  /*!
   * Returns the tessellation parameters used to construct
   * this TessellatedPath.
   */
  const TessellationParams&
  tessellation_parameters(void) const;

  /*!
   * Returns the tessellation threshold achieved
   */
  float
  effective_threshhold(void) const;

  /*!
   * Returns the maximum number of segments any edge needed
   */
  unsigned int
  max_segments(void) const;

  /*!
   * Returns all the segment data
   */
  c_array<const segment>
  segment_data(void) const;

  /*!
   * Returns the number of contours
   */
  unsigned int
  number_contours(void) const;

  /*!
   * Returns the range into segment_data() for the named
   * contour.
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   */
  range_type<unsigned int>
  contour_range(unsigned int contour) const;

  /*!
   * Returns the range into segment_data() for the named
   * contour lacking the closing edge.
   * replicated (because the derivatives are different).
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   */
  range_type<unsigned int>
  unclosed_contour_range(unsigned int contour) const;

  /*!
   * Returns the segment data of the named contour including
   * the closing edge. Provided as a conveniance equivalent to
   * \code
   * segment_data().sub_array(contour_range(contour))
   * \endcode
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   */
  c_array<const segment>
  contour_segment_data(unsigned int contour) const;

  /*!
   * Returns the segment data of the named contour
   * lacking the segment data of the closing edge.
   * Provided as a conveniance, equivalent to
   * \code
   * segment_data().sub_array(unclosed_contour_range(contour))
   * \endcode
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   */
  c_array<const segment>
  unclosed_contour_segment_data(unsigned int contour) const;

  /*!
   * Returns the number of edges for the named contour
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   */
  unsigned int
  number_edges(unsigned int contour) const;

  /*!
   * Returns the range into segment_data(void)
   * for the named edge of the named contour.
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   * \param edge which edge of the contour to query, must
   *             have that 0 <= edge < number_edges(contour)
   */
  range_type<unsigned int>
  edge_range(unsigned int contour, unsigned int edge) const;

  /*!
   * Returns the segment data of the named edge of the
   * named contour, provided as a conveniance, equivalent
   * to
   * \code
   * segment_data().sub_array(edge_range(contour, edge))
   * \endcode
   * \param contour which path contour to query, must have
   *                that 0 <= contour < number_contours()
   * \param edge which edge of the contour to query, must
   *             have that 0 <= edge < number_edges(contour)
   */
  c_array<const segment>
  edge_segment_data(unsigned int contour, unsigned int edge) const;

  /*!
   * Returns the minimum point of the bounding box of
   * the tessellation.
   */
  vec2
  bounding_box_min(void) const;

  /*!
   * Returns the maximum point of the bounding box of
   * the tessellation.
   */
  vec2
  bounding_box_max(void) const;

  /*!
   * Returns the dimensions of the bounding box
   * of the tessellated path.
   */
  vec2
  bounding_box_size(void) const;

  /*!
   * Returns this ArcTessellatedPath stroked. The ArcStrokedPath object
   * is constructed lazily.
   */
  const reference_counted_ptr<const ArcStrokedPath>&
  stroked(void) const;

private:
  void *m_d;
};

/*! @} */

}
