/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/layout/flex/flexible_box_algorithm.h"

#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/layout/flex/ng_flex_line.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/logical_box_fragment.h"
#include "third_party/blink/renderer/core/layout/min_max_sizes.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"

namespace blink {
namespace {

ItemPosition BoxAlignmentToItemPosition(EBoxAlignment alignment) {
  switch (alignment) {
    case EBoxAlignment::kBaseline:
      return ItemPosition::kBaseline;
    case EBoxAlignment::kCenter:
      return ItemPosition::kCenter;
    case EBoxAlignment::kStretch:
      return ItemPosition::kStretch;
    case EBoxAlignment::kStart:
      return ItemPosition::kFlexStart;
    case EBoxAlignment::kEnd:
      return ItemPosition::kFlexEnd;
  }
}

ContentPosition BoxPackToContentPosition(EBoxPack box_pack) {
  switch (box_pack) {
    case EBoxPack::kCenter:
      return ContentPosition::kCenter;
    case EBoxPack::kJustify:
      return ContentPosition::kFlexStart;
    case EBoxPack::kStart:
      return ContentPosition::kFlexStart;
    case EBoxPack::kEnd:
      return ContentPosition::kFlexEnd;
  }
}

ContentDistributionType BoxPackToContentDistribution(EBoxPack box_pack) {
  return box_pack == EBoxPack::kJustify ? ContentDistributionType::kSpaceBetween
                                        : ContentDistributionType::kDefault;
}

}  // namespace

FlexItem::FlexItem(const FlexibleBoxAlgorithm* algorithm,
                   const ComputedStyle& style,
                   unsigned main_axis_auto_margin_count,
                   LayoutUnit flex_base_content_size,
                   MinMaxSizes min_max_main_sizes,
                   LayoutUnit main_axis_border_padding,
                   PhysicalBoxStrut physical_margins,
                   BoxStrut scrollbars,
                   WritingMode baseline_writing_mode,
                   BaselineGroup baseline_group,
                   bool is_initial_block_size_indefinite,
                   bool is_used_flex_basis_indefinite,
                   bool depends_on_min_max_sizes)
    : algorithm_(algorithm),
      style_(style),
      flex_grow_(style.ResolvedFlexGrow(algorithm_->StyleRef())),
      flex_shrink_(style.ResolvedFlexShrink(algorithm_->StyleRef())),
      main_axis_auto_margin_count_(main_axis_auto_margin_count),
      flex_base_content_size_(flex_base_content_size),
      min_max_main_sizes_(min_max_main_sizes),
      hypothetical_main_content_size_(
          min_max_main_sizes.ClampSizeToMinAndMax(flex_base_content_size)),
      main_axis_border_padding_(main_axis_border_padding),
      physical_margins_(physical_margins),
      scrollbars_(scrollbars),
      baseline_writing_direction_({baseline_writing_mode, TextDirection::kLtr}),
      baseline_group_(baseline_group),
      is_initial_block_size_indefinite_(is_initial_block_size_indefinite),
      is_used_flex_basis_indefinite_(is_used_flex_basis_indefinite),
      depends_on_min_max_sizes_(depends_on_min_max_sizes),
      frozen_(false),
      ng_input_node_(/* LayoutBox* */ nullptr) {
  DCHECK_GE(min_max_main_sizes.max_size, LayoutUnit())
      << "Use LayoutUnit::Max() for no max size";
}

LayoutUnit FlexItem::FlowAwareMarginBefore() const {
  switch (algorithm_->CrossAxisDirection()) {
    case PhysicalDirection::kDown:
      return physical_margins_.top;
    case PhysicalDirection::kUp:
      return physical_margins_.bottom;
    case PhysicalDirection::kRight:
      return physical_margins_.left;
    case PhysicalDirection::kLeft:
      return physical_margins_.right;
  }
  NOTREACHED();
}

LayoutUnit FlexItem::FlowAwareMarginAfter() const {
  switch (algorithm_->CrossAxisDirection()) {
    case PhysicalDirection::kDown:
      return physical_margins_.bottom;
    case PhysicalDirection::kUp:
      return physical_margins_.top;
    case PhysicalDirection::kRight:
      return physical_margins_.right;
    case PhysicalDirection::kLeft:
      return physical_margins_.left;
  }
  NOTREACHED();
}

LayoutUnit FlexItem::MainAxisMarginExtent() const {
  return algorithm_->IsHorizontalFlow() ? physical_margins_.HorizontalSum()
                                        : physical_margins_.VerticalSum();
}

LayoutUnit FlexItem::CrossAxisMarginExtent() const {
  return algorithm_->IsHorizontalFlow() ? physical_margins_.VerticalSum()
                                        : physical_margins_.HorizontalSum();
}

LayoutUnit FlexItem::MarginBoxAscent(bool is_last_baseline,
                                     bool is_wrap_reverse) const {
  DCHECK(layout_result_);
  LogicalBoxFragment baseline_fragment(
      baseline_writing_direction_,
      To<PhysicalBoxFragment>(layout_result_->GetPhysicalFragment()));

  const auto font_baseline = algorithm_->StyleRef().GetFontBaseline();
  LayoutUnit baseline =
      is_last_baseline
          ? baseline_fragment.LastBaselineOrSynthesize(font_baseline)
          : baseline_fragment.FirstBaselineOrSynthesize(font_baseline);
  if (is_wrap_reverse != is_last_baseline)
    baseline = baseline_fragment.BlockSize() - baseline;

  return baseline_group_ == BaselineGroup::kMajor
             ? FlowAwareMarginBefore() + baseline
             : FlowAwareMarginAfter() + baseline;
}

ItemPosition FlexItem::Alignment() const {
  return FlexibleBoxAlgorithm::AlignmentForChild(*algorithm_->Style(), *style_);
}

LayoutUnit FlexItem::CrossAxisOffset(const NGFlexLine& line,
                                     LayoutUnit cross_axis_size) {
  LayoutUnit available_space =
      line.line_cross_size - (CrossAxisMarginExtent() + cross_axis_size);

  const auto* parent_style = algorithm_->Style();
  const bool is_webkit_box = parent_style->IsDeprecatedWebkitBox();
  const bool is_wrap_reverse =
      parent_style->FlexWrap() == EFlexWrap::kWrapReverse;
  const ItemPosition position = Alignment();
  if (!is_webkit_box && style_->ResolvedAlignSelf({ItemPosition::kStretch,
                                                   OverflowAlignment::kDefault},
                                                  parent_style)
                                .Overflow() == OverflowAlignment::kSafe) {
    available_space = available_space.ClampNegativeToZero();
  }

  LayoutUnit baseline_offset;
  if (position == ItemPosition::kBaseline ||
      position == ItemPosition::kLastBaseline) {
    bool is_major = baseline_group_ == BaselineGroup::kMajor;
    LayoutUnit ascent = MarginBoxAscent(position == ItemPosition::kLastBaseline,
                                        is_wrap_reverse);
    LayoutUnit max_ascent =
        is_major ? line.major_baseline : line.minor_baseline;

    LayoutUnit baseline_delta = max_ascent - ascent;
    baseline_offset =
        is_major ? baseline_delta : available_space - baseline_delta;
  }
  return FlexItem::AlignmentOffset(available_space, position, baseline_offset,
                                   is_wrap_reverse);
}

void FlexItem::Trace(Visitor* visitor) const {
  visitor->Trace(style_);
  visitor->Trace(ng_input_node_);
  visitor->Trace(layout_result_);
}

// static
LayoutUnit FlexItem::AlignmentOffset(LayoutUnit available_free_space,
                                     ItemPosition position,
                                     LayoutUnit baseline_offset,
                                     bool is_wrap_reverse) {
  switch (position) {
    case ItemPosition::kLegacy:
    case ItemPosition::kAuto:
    case ItemPosition::kNormal:
    case ItemPosition::kAnchorCenter:
      NOTREACHED();
    case ItemPosition::kSelfStart:
    case ItemPosition::kSelfEnd:
    case ItemPosition::kStart:
    case ItemPosition::kEnd:
    case ItemPosition::kLeft:
    case ItemPosition::kRight:
      NOTREACHED() << static_cast<int>(position)
                   << " AlignmentForChild should have transformed this "
                      "position value to something we handle below.";
    case ItemPosition::kStretch:
      // Actual stretching must be handled by the caller. Since wrap-reverse
      // flips cross start and cross end, stretch children should be aligned
      // with the cross end. This matters because applyStretchAlignment
      // doesn't always stretch or stretch fully (explicit cross size given, or
      // stretching constrained by max-height/max-width). For flex-start and
      // flex-end this is handled by alignmentForChild().
      if (is_wrap_reverse)
        return available_free_space;
      break;
    case ItemPosition::kFlexStart:
      break;
    case ItemPosition::kFlexEnd:
      return available_free_space;
    case ItemPosition::kCenter:
      return available_free_space / 2;
    case ItemPosition::kBaseline:
    case ItemPosition::kLastBaseline:
      return baseline_offset;
  }
  return LayoutUnit();
}

void FlexLine::FreezeViolations(ViolationsVector& violations) {
  for (auto* violation : violations) {
    DCHECK(!violation->frozen_);
    remaining_free_space_ -=
        violation->flexed_content_size_ - violation->flex_base_content_size_;
    total_flex_grow_ -= violation->flex_grow_;
    total_flex_shrink_ -= violation->flex_shrink_;
    total_weighted_flex_shrink_ -=
        violation->flex_shrink_ * violation->flex_base_content_size_;
    // total_weighted_flex_shrink can be negative when we exceed the precision
    // of a double when we initially calculate total_weighted_flex_shrink. We
    // then subtract each child's weighted flex shrink with full precision, now
    // leading to a negative result. See
    // css3/flexbox/large-flex-shrink-assert.html
    total_weighted_flex_shrink_ = std::max(total_weighted_flex_shrink_, 0.0);
    violation->frozen_ = true;
  }
}

void FlexLine::FreezeInflexibleItems() {
  // Per https://drafts.csswg.org/css-flexbox/#resolve-flexible-lengths step 2,
  // we freeze all items with a flex factor of 0 as well as those with a min/max
  // size violation.
  const FlexSign flex_sign = Sign();
  remaining_free_space_ = container_main_inner_size_ - sum_flex_base_size_;

  ViolationsVector new_inflexible_items;
  for (auto& flex_item : line_items_) {
    DCHECK(!flex_item.frozen_);
    float flex_factor = (flex_sign == kPositiveFlexibility)
                            ? flex_item.flex_grow_
                            : flex_item.flex_shrink_;
    if (flex_factor == 0 ||
        (flex_sign == kPositiveFlexibility &&
         flex_item.flex_base_content_size_ >
             flex_item.hypothetical_main_content_size_) ||
        (flex_sign == kNegativeFlexibility &&
         flex_item.flex_base_content_size_ <
             flex_item.hypothetical_main_content_size_)) {
      flex_item.flexed_content_size_ =
          flex_item.hypothetical_main_content_size_;
      new_inflexible_items.push_back(&flex_item);
    }
  }
  FreezeViolations(new_inflexible_items);
  initial_free_space_ = remaining_free_space_;
}

bool FlexLine::ResolveFlexibleLengths() {
  LayoutUnit total_violation;
  LayoutUnit used_free_space;
  ViolationsVector min_violations;
  ViolationsVector max_violations;

  const FlexSign flex_sign = Sign();
  const double sum_flex_factors = (flex_sign == kPositiveFlexibility)
                                      ? total_flex_grow_
                                      : total_flex_shrink_;
  if (sum_flex_factors > 0 && sum_flex_factors < 1) {
    LayoutUnit fractional(initial_free_space_ * sum_flex_factors);
    if (fractional.Abs() < remaining_free_space_.Abs())
      remaining_free_space_ = fractional;
  }

  for (auto& flex_item : line_items_) {
    if (flex_item.frozen_)
      continue;

    LayoutUnit child_size = flex_item.flex_base_content_size_;
    double extra_space = 0;
    if (remaining_free_space_ > 0 && total_flex_grow_ > 0 &&
        flex_sign == kPositiveFlexibility && std::isfinite(total_flex_grow_)) {
      extra_space =
          remaining_free_space_ * flex_item.flex_grow_ / total_flex_grow_;
    } else if (remaining_free_space_ < 0 && total_weighted_flex_shrink_ > 0 &&
               flex_sign == kNegativeFlexibility &&
               std::isfinite(total_weighted_flex_shrink_) &&
               flex_item.flex_shrink_) {
      extra_space = remaining_free_space_ * flex_item.flex_shrink_ *
                    flex_item.flex_base_content_size_ /
                    total_weighted_flex_shrink_;
    }
    if (std::isfinite(extra_space))
      child_size += LayoutUnit::FromFloatRound(extra_space);

    LayoutUnit adjusted_child_size = flex_item.ClampSizeToMinAndMax(child_size);
    DCHECK_GE(adjusted_child_size, 0);
    flex_item.flexed_content_size_ = adjusted_child_size;
    used_free_space += adjusted_child_size - flex_item.flex_base_content_size_;

    LayoutUnit violation = adjusted_child_size - child_size;
    if (violation > 0)
      min_violations.push_back(&flex_item);
    else if (violation < 0)
      max_violations.push_back(&flex_item);
    total_violation += violation;
  }

  if (total_violation) {
    FreezeViolations(total_violation < 0 ? max_violations : min_violations);
  } else {
    remaining_free_space_ -= used_free_space;
  }

  return !total_violation;
}

void FlexLine::ComputeLineItemsPosition() {
  const auto& style = algorithm_->StyleRef();
  const bool is_wrap_reverse = style.FlexWrap() == EFlexWrap::kWrapReverse;

  // Recalculate the remaining free space. The adjustment for flex factors
  // between 0..1 means we can't just use remainingFreeSpace here.
  LayoutUnit total_item_size;
  for (wtf_size_t i = 0; i < line_items_.size(); ++i)
    total_item_size += line_items_[i].FlexedMarginBoxSize();
  remaining_free_space_ =
      container_main_inner_size_ - total_item_size -
      (line_items_.size() - 1) * algorithm_->gap_between_items_;

  LayoutUnit max_major_descent = LayoutUnit::Min();
  LayoutUnit max_minor_descent = LayoutUnit::Min();

  LayoutUnit max_child_cross_axis_extent;
  for (wtf_size_t i = 0; i < line_items_.size(); ++i) {
    FlexItem& flex_item = line_items_[i];

    LayoutUnit child_cross_axis_margin_box_extent;
    // TODO(crbug.com/1272533): We may not have a layout-result during min/max
    // calculations. This is incorrect, and we should produce a layout-result
    // when baseline aligned.
    const auto alignment = flex_item.Alignment();
    if (flex_item.layout_result_ &&
        (alignment == ItemPosition::kBaseline ||
         alignment == ItemPosition::kLastBaseline)) {
      LayoutUnit ascent = flex_item.MarginBoxAscent(
          alignment == ItemPosition::kLastBaseline, is_wrap_reverse);
      LayoutUnit descent =
          (flex_item.CrossAxisMarginExtent() + flex_item.cross_axis_size_) -
          ascent;
      if (flex_item.baseline_group_ == BaselineGroup::kMajor) {
        max_major_ascent_ = std::max(max_major_ascent_, ascent);
        max_major_descent = std::max(max_major_descent, descent);
        child_cross_axis_margin_box_extent =
            max_major_ascent_ + max_major_descent;
      } else {
        max_minor_ascent_ = std::max(max_minor_ascent_, ascent);
        max_minor_descent = std::max(max_minor_descent, descent);
        child_cross_axis_margin_box_extent =
            max_minor_ascent_ + max_minor_descent;
      }
    } else {
      child_cross_axis_margin_box_extent =
          flex_item.cross_axis_size_ + flex_item.CrossAxisMarginExtent();
    }
    max_child_cross_axis_extent = std::max(max_child_cross_axis_extent,
                                           child_cross_axis_margin_box_extent);
  }
  cross_axis_extent_ = max_child_cross_axis_extent;
}

// static
LayoutUnit FlexibleBoxAlgorithm::GapBetweenItems(
    const ComputedStyle& style,
    LogicalSize percent_resolution_sizes) {
  if (IsColumnFlow(style)) {
    if (const std::optional<Length>& row_gap = style.RowGap()) {
      return MinimumValueForLength(
          *row_gap,
          percent_resolution_sizes.block_size.ClampIndefiniteToZero());
    }
    return LayoutUnit();
  }
  if (const std::optional<Length>& column_gap = style.ColumnGap()) {
    return MinimumValueForLength(
        *column_gap,
        percent_resolution_sizes.inline_size.ClampIndefiniteToZero());
  }
  return LayoutUnit();
}

// static
LayoutUnit FlexibleBoxAlgorithm::GapBetweenLines(
    const ComputedStyle& style,
    LogicalSize percent_resolution_sizes) {
  if (!IsColumnFlow(style)) {
    if (const std::optional<Length>& row_gap = style.RowGap()) {
      return MinimumValueForLength(
          *row_gap,
          percent_resolution_sizes.block_size.ClampIndefiniteToZero());
    }
    return LayoutUnit();
  }
  if (const std::optional<Length>& column_gap = style.ColumnGap()) {
    return MinimumValueForLength(
        *column_gap,
        percent_resolution_sizes.inline_size.ClampIndefiniteToZero());
  }
  return LayoutUnit();
}

FlexibleBoxAlgorithm::FlexibleBoxAlgorithm(const ComputedStyle* style,
                                           LayoutUnit line_break_length,
                                           LogicalSize percent_resolution_sizes,
                                           Document* document)
    : gap_between_items_(GapBetweenItems(*style, percent_resolution_sizes)),
      gap_between_lines_(GapBetweenLines(*style, percent_resolution_sizes)),
      style_(style),
      line_break_length_(line_break_length),
      next_item_index_(0) {
  DCHECK_GE(gap_between_items_, 0);
  DCHECK_GE(gap_between_lines_, 0);
  const auto& row_gap = style->RowGap();
  const auto& column_gap = style->ColumnGap();
  if (row_gap || column_gap) {
    UseCounter::Count(document, WebFeature::kFlexGapSpecified);
    if (gap_between_items_ || gap_between_lines_)
      UseCounter::Count(document, WebFeature::kFlexGapPositive);
  }

  if (row_gap && row_gap->HasPercent()) {
    UseCounter::Count(document, WebFeature::kFlexRowGapPercent);
    if (percent_resolution_sizes.block_size == LayoutUnit(-1))
      UseCounter::Count(document, WebFeature::kFlexRowGapPercentIndefinite);
  }
}

FlexLine* FlexibleBoxAlgorithm::ComputeNextFlexLine() {
  LayoutUnit sum_flex_base_size;
  LayoutUnit sum_hypothetical_main_size;
  double total_flex_grow = 0;
  double total_flex_shrink = 0;
  double total_weighted_flex_shrink = 0;
  unsigned main_axis_auto_margin_count = 0;

  bool line_has_in_flow_item = false;

  wtf_size_t start_index = next_item_index_;

  for (; next_item_index_ < all_items_.size(); ++next_item_index_) {
    FlexItem& flex_item = all_items_[next_item_index_];
    if (IsMultiline() &&
        sum_hypothetical_main_size +
                flex_item.HypotheticalMainAxisMarginBoxSize() >
            line_break_length_ &&
        line_has_in_flow_item) {
      break;
    }
    line_has_in_flow_item = true;
    sum_flex_base_size +=
        flex_item.FlexBaseMarginBoxSize() + gap_between_items_;
    sum_hypothetical_main_size +=
        flex_item.HypotheticalMainAxisMarginBoxSize() + gap_between_items_;
    total_flex_grow += flex_item.flex_grow_;
    total_flex_shrink += flex_item.flex_shrink_;
    total_weighted_flex_shrink +=
        flex_item.flex_shrink_ * flex_item.flex_base_content_size_;
    main_axis_auto_margin_count += flex_item.main_axis_auto_margin_count_;
  }
  if (line_has_in_flow_item) {
    // We added a gap after every item but there shouldn't be one after the last
    // item, so subtract it here.
    // Note: the two sums here can be negative because of negative margins.
    sum_hypothetical_main_size -= gap_between_items_;
    sum_flex_base_size -= gap_between_items_;
  }

  DCHECK(next_item_index_ > start_index ||
         next_item_index_ == all_items_.size());
  if (next_item_index_ > start_index) {
    return &flex_lines_.emplace_back(
        this, FlexItemVectorView(&all_items_, start_index, next_item_index_),
        sum_flex_base_size, sum_hypothetical_main_size, total_flex_grow,
        total_flex_shrink, total_weighted_flex_shrink,
        main_axis_auto_margin_count);
  }
  return nullptr;
}

bool FlexibleBoxAlgorithm::IsHorizontalFlow() const {
  return IsHorizontalFlow(*style_);
}

bool FlexibleBoxAlgorithm::IsColumnFlow() const {
  return IsColumnFlow(*style_);
}

// static
bool FlexibleBoxAlgorithm::IsColumnFlow(const ComputedStyle& style) {
  return style.ResolvedIsColumnFlexDirection();
}

// static
bool FlexibleBoxAlgorithm::IsHorizontalFlow(const ComputedStyle& style) {
  if (style.IsHorizontalWritingMode())
    return !style.ResolvedIsColumnFlexDirection();
  return style.ResolvedIsColumnFlexDirection();
}

// static
const StyleContentAlignmentData&
FlexibleBoxAlgorithm::ContentAlignmentNormalBehavior() {
  // The justify-content property applies along the main axis, but since
  // flexing in the main axis is controlled by flex, stretch behaves as
  // flex-start (ignoring the specified fallback alignment, if any).
  // https://drafts.csswg.org/css-align/#distribution-flex
  static const StyleContentAlignmentData kNormalBehavior = {
      ContentPosition::kNormal, ContentDistributionType::kStretch};
  return kNormalBehavior;
}

bool FlexibleBoxAlgorithm::ShouldApplyMinSizeAutoForChild(
    const LayoutBox& child) const {
  // See: https://drafts.csswg.org/css-flexbox/#min-size-auto

  // webkit-box treats min-size: auto as 0.
  if (StyleRef().IsDeprecatedWebkitBox()) {
    return false;
  }

  if (child.ShouldApplySizeContainment()) {
    return false;
  }

  // Note that the spec uses "scroll container", but it's resolved to just look
  // at the computed value of overflow not being scrollable, see
  // https://github.com/w3c/csswg-drafts/issues/7714#issuecomment-1879319762
  if (child.StyleRef().IsScrollContainer()) {
    return false;
  }

  const Length& min = IsHorizontalFlow() ? child.StyleRef().MinWidth()
                                         : child.StyleRef().MinHeight();
  return min.HasAuto();
}

PhysicalDirection FlexibleBoxAlgorithm::CrossAxisDirection() const {
  const WritingDirectionMode writing_direction = style_->GetWritingDirection();
  return style_->ResolvedIsColumnFlexDirection() ? writing_direction.InlineEnd()
                                                 : writing_direction.BlockEnd();
}

// static
StyleContentAlignmentData FlexibleBoxAlgorithm::ResolvedJustifyContent(
    const ComputedStyle& style) {
  const bool is_webkit_box = style.IsDeprecatedWebkitBox();
  ContentPosition position;
  if (is_webkit_box) {
    position = BoxPackToContentPosition(style.BoxPack());
    // As row-reverse does layout in reverse, it effectively swaps end & start.
    // -webkit-box didn't do this (-webkit-box always did layout starting at 0,
    // and increasing).
    if (style.ResolvedIsRowReverseFlexDirection()) {
      if (position == ContentPosition::kFlexEnd)
        position = ContentPosition::kFlexStart;
      else if (position == ContentPosition::kFlexStart)
        position = ContentPosition::kFlexEnd;
    }
  } else {
    position =
        style.ResolvedJustifyContentPosition(ContentAlignmentNormalBehavior());
  }
  if (position == ContentPosition::kLeft ||
      position == ContentPosition::kRight) {
    if (IsColumnFlow(style)) {
      if (style.IsHorizontalWritingMode()) {
        // Main axis is perpendicular to both the physical left<->right and
        // inline start<->end axes, so kLeft and kRight behave as kStart.
        position = ContentPosition::kStart;
      } else if ((position == ContentPosition::kLeft &&
                  style.IsFlippedBlocksWritingMode()) ||
                 (position == ContentPosition::kRight &&
                  style.GetWritingDirection().BlockEnd() ==
                      PhysicalDirection::kRight)) {
        position = ContentPosition::kEnd;
      } else {
        position = ContentPosition::kStart;
      }
    } else if ((position == ContentPosition::kLeft &&
                !style.IsLeftToRightDirection()) ||
               (position == ContentPosition::kRight &&
                style.IsLeftToRightDirection())) {
      DCHECK(!FlexibleBoxAlgorithm::IsColumnFlow(style));
      position = ContentPosition::kEnd;
    } else {
      position = ContentPosition::kStart;
    }
  }
  DCHECK_NE(position, ContentPosition::kLeft);
  DCHECK_NE(position, ContentPosition::kRight);

  ContentDistributionType distribution =
      is_webkit_box ? BoxPackToContentDistribution(style.BoxPack())
                    : style.ResolvedJustifyContentDistribution(
                          ContentAlignmentNormalBehavior());
  OverflowAlignment overflow = style.JustifyContent().Overflow();
  if (is_webkit_box) {
    overflow = OverflowAlignment::kSafe;
  } else if (distribution == ContentDistributionType::kStretch) {
    // For flex, justify-content: stretch behaves as flex-start:
    // https://drafts.csswg.org/css-align/#distribution-flex
    position = ContentPosition::kFlexStart;
    distribution = ContentDistributionType::kDefault;
  }
  return StyleContentAlignmentData(position, distribution, overflow);
}

// static
StyleContentAlignmentData FlexibleBoxAlgorithm::ResolvedAlignContent(
    const ComputedStyle& style) {
  ContentPosition position =
      style.ResolvedAlignContentPosition(ContentAlignmentNormalBehavior());
  ContentDistributionType distribution =
      style.ResolvedAlignContentDistribution(ContentAlignmentNormalBehavior());
  OverflowAlignment overflow = style.AlignContent().Overflow();
  return StyleContentAlignmentData(position, distribution, overflow);
}

// static
ItemPosition FlexibleBoxAlgorithm::AlignmentForChild(
    const ComputedStyle& flexbox_style,
    const ComputedStyle& child_style) {
  ItemPosition align =
      flexbox_style.IsDeprecatedWebkitBox()
          ? BoxAlignmentToItemPosition(flexbox_style.BoxAlign())
          : child_style
                .ResolvedAlignSelf(
                    {ItemPosition::kStretch, OverflowAlignment::kDefault},
                    &flexbox_style)
                .GetPosition();
  DCHECK_NE(align, ItemPosition::kAuto);
  DCHECK_NE(align, ItemPosition::kNormal);
  DCHECK_NE(align, ItemPosition::kLeft) << "left, right are only for justify";
  DCHECK_NE(align, ItemPosition::kRight) << "left, right are only for justify";

  if (align == ItemPosition::kStart)
    return ItemPosition::kFlexStart;
  if (align == ItemPosition::kEnd)
    return ItemPosition::kFlexEnd;

  if (align == ItemPosition::kSelfStart || align == ItemPosition::kSelfEnd) {
    LogicalToPhysical<ItemPosition> physical(
        child_style.GetWritingDirection(), ItemPosition::kFlexStart,
        ItemPosition::kFlexEnd, ItemPosition::kFlexStart,
        ItemPosition::kFlexEnd);

    PhysicalToLogical<ItemPosition> logical(flexbox_style.GetWritingDirection(),
                                            physical.Top(), physical.Right(),
                                            physical.Bottom(), physical.Left());

    if (flexbox_style.ResolvedIsColumnFlexDirection()) {
      return align == ItemPosition::kSelfStart ? logical.InlineStart()
                                               : logical.InlineEnd();
    }
    return align == ItemPosition::kSelfStart ? logical.BlockStart()
                                             : logical.BlockEnd();
  }

  if (flexbox_style.FlexWrap() == EFlexWrap::kWrapReverse) {
    if (align == ItemPosition::kFlexStart) {
      align = ItemPosition::kFlexEnd;
    } else if (align == ItemPosition::kFlexEnd) {
      align = ItemPosition::kFlexStart;
    }
  }

  if (!child_style.HasOutOfFlowPosition()) {
    if (IsHorizontalFlow(flexbox_style)) {
      if (child_style.MarginTop().IsAuto() ||
          child_style.MarginBottom().IsAuto()) {
        align = ItemPosition::kFlexStart;
      }
    } else {
      if (child_style.MarginLeft().IsAuto() ||
          child_style.MarginRight().IsAuto()) {
        align = ItemPosition::kFlexStart;
      }
    }
  }

  return align;
}

// static
LayoutUnit FlexibleBoxAlgorithm::ContentDistributionSpaceBetweenChildren(
    LayoutUnit available_free_space,
    const StyleContentAlignmentData& data,
    unsigned number_of_items) {
  if (available_free_space > 0 && number_of_items > 1) {
    if (data.Distribution() == ContentDistributionType::kSpaceBetween)
      return available_free_space / (number_of_items - 1);
    if (data.Distribution() == ContentDistributionType::kSpaceAround ||
        data.Distribution() == ContentDistributionType::kStretch)
      return available_free_space / number_of_items;
    if (data.Distribution() == ContentDistributionType::kSpaceEvenly)
      return available_free_space / (number_of_items + 1);
  }
  return LayoutUnit();
}

FlexItem* FlexibleBoxAlgorithm::FlexItemAtIndex(wtf_size_t line_index,
                                                wtf_size_t item_index) const {
  DCHECK_LT(line_index, flex_lines_.size());
  if (StyleRef().FlexWrap() == EFlexWrap::kWrapReverse)
    line_index = flex_lines_.size() - line_index - 1;

  DCHECK_LT(item_index, flex_lines_[line_index].line_items_.size());
  if (Style()->ResolvedIsReverseFlexDirection()) {
    item_index = flex_lines_[line_index].line_items_.size() - item_index - 1;
  }
  return const_cast<FlexItem*>(
      &flex_lines_[line_index].line_items_[item_index]);
}

void FlexibleBoxAlgorithm::Trace(Visitor* visitor) const {
  visitor->Trace(style_);
  visitor->Trace(all_items_);
}

}  // namespace blink
