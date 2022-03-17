// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/cpdfsdk_appstream.h"

#include <math.h>

#include <memory>
#include <sstream>
#include <utility>

#include "constants/appearance.h"
#include "constants/form_flags.h"
#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fpdfdoc/cpdf_bafontmap.h"
#include "core/fpdfdoc/cpdf_formcontrol.h"
#include "core/fpdfdoc/cpdf_icon.h"
#include "core/fpdfdoc/cpvt_word.h"
#include "core/fxcrt/fx_string_wrappers.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_interactiveform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/cpdfsdk_widget.h"
#include "fpdfsdk/pwl/cpwl_edit.h"
#include "fpdfsdk/pwl/cpwl_edit_impl.h"
#include "fpdfsdk/pwl/cpwl_wnd.h"
#include "third_party/base/cxx17_backports.h"
#include "third_party/base/numerics/safe_conversions.h"

namespace {

// Checkbox & radiobutton styles.
enum class CheckStyle { kCheck = 0, kCircle, kCross, kDiamond, kSquare, kStar };

// Pushbutton layout styles.
enum class ButtonStyle {
  kLabel = 0,
  kIcon,
  kIconTopLabelBottom,
  kIconBottomLabelTop,
  kIconLeftLabelRight,
  kIconRightLabelLeft,
  kLabelOverIcon
};

const char kAppendRectOperator[] = "re";
const char kConcatMatrixOperator[] = "cm";
const char kCurveToOperator[] = "c";
const char kEndPathNoFillOrStrokeOperator[] = "n";
const char kFillOperator[] = "f";
const char kFillEvenOddOperator[] = "f*";
const char kInvokeNamedXObjectOperator[] = "Do";
const char kLineToOperator[] = "l";
const char kMarkedSequenceBeginOperator[] = "BMC";
const char kMarkedSequenceEndOperator[] = "EMC";
const char kMoveTextPositionOperator[] = "Td";
const char kMoveToOperator[] = "m";
const char kSetCMYKOperator[] = "k";
const char kSetCMKYStrokedOperator[] = "K";
const char kSetDashOperator[] = "d";
const char kSetGrayOperator[] = "g";
const char kSetGrayStrokedOperator[] = "G";
const char kSetLineCapStyleOperator[] = "J";
const char kSetLineJoinStyleOperator[] = "j";
const char kSetLineWidthOperator[] = "w";
const char kSetNonZeroWindingClipOperator[] = "W";
const char kSetRGBOperator[] = "rg";
const char kSetRGBStrokedOperator[] = "RG";
const char kSetTextFontAndSizeOperator[] = "Tf";
const char kShowTextOperator[] = "Tj";
const char kStateRestoreOperator[] = "Q";
const char kStateSaveOperator[] = "q";
const char kStrokeOperator[] = "S";
const char kTextBeginOperator[] = "BT";
const char kTextEndOperator[] = "ET";

class AutoClosedCommand {
 public:
  AutoClosedCommand(fxcrt::ostringstream* stream,
                    ByteString open,
                    ByteString close)
      : stream_(stream), close_(close) {
    *stream_ << open << "\n";
  }

  virtual ~AutoClosedCommand() { *stream_ << close_ << "\n"; }

 private:
  UnownedPtr<fxcrt::ostringstream> const stream_;
  ByteString close_;
};

class AutoClosedQCommand final : public AutoClosedCommand {
 public:
  explicit AutoClosedQCommand(fxcrt::ostringstream* stream)
      : AutoClosedCommand(stream, kStateSaveOperator, kStateRestoreOperator) {}
  ~AutoClosedQCommand() override = default;
};

ByteString GetColorAppStream(const CFX_Color& color,
                             const bool& bFillOrStroke) {
  fxcrt::ostringstream sColorStream;
  switch (color.nColorType) {
    case CFX_Color::Type::kTransparent:
      break;
    case CFX_Color::Type::kGray:
      sColorStream << color.fColor1 << " "
                   << (bFillOrStroke ? kSetGrayOperator
                                     : kSetGrayStrokedOperator)
                   << "\n";
      break;
    case CFX_Color::Type::kRGB:
      sColorStream << color.fColor1 << " " << color.fColor2 << " "
                   << color.fColor3 << " "
                   << (bFillOrStroke ? kSetRGBOperator : kSetRGBStrokedOperator)
                   << "\n";
      break;
    case CFX_Color::Type::kCMYK:
      sColorStream << color.fColor1 << " " << color.fColor2 << " "
                   << color.fColor3 << " " << color.fColor4 << " "
                   << (bFillOrStroke ? kSetCMYKOperator
                                     : kSetCMKYStrokedOperator)
                   << "\n";
      break;
  }

  return ByteString(sColorStream);
}

ByteString GetAP_Check(const CFX_FloatRect& crBBox) {
  const float fWidth = crBBox.Width();
  const float fHeight = crBBox.Height();

  CFX_PointF pts[8][3] = {{CFX_PointF(0.28f, 0.52f), CFX_PointF(0.27f, 0.48f),
                           CFX_PointF(0.29f, 0.40f)},
                          {CFX_PointF(0.30f, 0.33f), CFX_PointF(0.31f, 0.29f),
                           CFX_PointF(0.31f, 0.28f)},
                          {CFX_PointF(0.39f, 0.28f), CFX_PointF(0.49f, 0.29f),
                           CFX_PointF(0.77f, 0.67f)},
                          {CFX_PointF(0.76f, 0.68f), CFX_PointF(0.78f, 0.69f),
                           CFX_PointF(0.76f, 0.75f)},
                          {CFX_PointF(0.76f, 0.75f), CFX_PointF(0.73f, 0.80f),
                           CFX_PointF(0.68f, 0.75f)},
                          {CFX_PointF(0.68f, 0.74f), CFX_PointF(0.68f, 0.74f),
                           CFX_PointF(0.44f, 0.47f)},
                          {CFX_PointF(0.43f, 0.47f), CFX_PointF(0.40f, 0.47f),
                           CFX_PointF(0.41f, 0.58f)},
                          {CFX_PointF(0.40f, 0.60f), CFX_PointF(0.28f, 0.66f),
                           CFX_PointF(0.30f, 0.56f)}};

  for (size_t i = 0; i < pdfium::size(pts); ++i) {
    for (size_t j = 0; j < pdfium::size(pts[0]); ++j) {
      pts[i][j].x = pts[i][j].x * fWidth + crBBox.left;
      pts[i][j].y *= pts[i][j].y * fHeight + crBBox.bottom;
    }
  }

  fxcrt::ostringstream csAP;
  csAP << pts[0][0].x << " " << pts[0][0].y << " " << kMoveToOperator << "\n";

  for (size_t i = 0; i < pdfium::size(pts); ++i) {
    size_t nNext = i < pdfium::size(pts) - 1 ? i + 1 : 0;

    float px1 = pts[i][1].x - pts[i][0].x;
    float py1 = pts[i][1].y - pts[i][0].y;
    float px2 = pts[i][2].x - pts[nNext][0].x;
    float py2 = pts[i][2].y - pts[nNext][0].y;

    csAP << pts[i][0].x + px1 * FXSYS_BEZIER << " "
         << pts[i][0].y + py1 * FXSYS_BEZIER << " "
         << pts[nNext][0].x + px2 * FXSYS_BEZIER << " "
         << pts[nNext][0].y + py2 * FXSYS_BEZIER << " " << pts[nNext][0].x
         << " " << pts[nNext][0].y << " " << kCurveToOperator << "\n";
  }

  return ByteString(csAP);
}

ByteString GetAP_Circle(const CFX_FloatRect& crBBox) {
  fxcrt::ostringstream csAP;

  float fWidth = crBBox.Width();
  float fHeight = crBBox.Height();

  CFX_PointF pt1(crBBox.left, crBBox.bottom + fHeight / 2);
  CFX_PointF pt2(crBBox.left + fWidth / 2, crBBox.top);
  CFX_PointF pt3(crBBox.right, crBBox.bottom + fHeight / 2);
  CFX_PointF pt4(crBBox.left + fWidth / 2, crBBox.bottom);

  csAP << pt1.x << " " << pt1.y << " " << kMoveToOperator << "\n";

  float px = pt2.x - pt1.x;
  float py = pt2.y - pt1.y;

  csAP << pt1.x << " " << pt1.y + py * FXSYS_BEZIER << " "
       << pt2.x - px * FXSYS_BEZIER << " " << pt2.y << " " << pt2.x << " "
       << pt2.y << " " << kCurveToOperator << "\n";

  px = pt3.x - pt2.x;
  py = pt2.y - pt3.y;

  csAP << pt2.x + px * FXSYS_BEZIER << " " << pt2.y << " " << pt3.x << " "
       << pt3.y + py * FXSYS_BEZIER << " " << pt3.x << " " << pt3.y << " "
       << kCurveToOperator << "\n";

  px = pt3.x - pt4.x;
  py = pt3.y - pt4.y;

  csAP << pt3.x << " " << pt3.y - py * FXSYS_BEZIER << " "
       << pt4.x + px * FXSYS_BEZIER << " " << pt4.y << " " << pt4.x << " "
       << pt4.y << " " << kCurveToOperator << "\n";

  px = pt4.x - pt1.x;
  py = pt1.y - pt4.y;

  csAP << pt4.x - px * FXSYS_BEZIER << " " << pt4.y << " " << pt1.x << " "
       << pt1.y - py * FXSYS_BEZIER << " " << pt1.x << " " << pt1.y << " "
       << kCurveToOperator << "\n";

  return ByteString(csAP);
}

ByteString GetAP_Cross(const CFX_FloatRect& crBBox) {
  fxcrt::ostringstream csAP;

  csAP << crBBox.left << " " << crBBox.top << " " << kMoveToOperator << "\n";
  csAP << crBBox.right << " " << crBBox.bottom << " " << kLineToOperator
       << "\n";
  csAP << crBBox.left << " " << crBBox.bottom << " " << kMoveToOperator << "\n";
  csAP << crBBox.right << " " << crBBox.top << " " << kLineToOperator << "\n";

  return ByteString(csAP);
}

ByteString GetAP_Diamond(const CFX_FloatRect& crBBox) {
  fxcrt::ostringstream csAP;

  float fWidth = crBBox.Width();
  float fHeight = crBBox.Height();

  CFX_PointF pt1(crBBox.left, crBBox.bottom + fHeight / 2);
  CFX_PointF pt2(crBBox.left + fWidth / 2, crBBox.top);
  CFX_PointF pt3(crBBox.right, crBBox.bottom + fHeight / 2);
  CFX_PointF pt4(crBBox.left + fWidth / 2, crBBox.bottom);

  csAP << pt1.x << " " << pt1.y << " " << kMoveToOperator << "\n";
  csAP << pt2.x << " " << pt2.y << " " << kLineToOperator << "\n";
  csAP << pt3.x << " " << pt3.y << " " << kLineToOperator << "\n";
  csAP << pt4.x << " " << pt4.y << " " << kLineToOperator << "\n";
  csAP << pt1.x << " " << pt1.y << " " << kLineToOperator << "\n";

  return ByteString(csAP);
}

ByteString GetAP_Square(const CFX_FloatRect& crBBox) {
  fxcrt::ostringstream csAP;

  csAP << crBBox.left << " " << crBBox.top << " " << kMoveToOperator << "\n";
  csAP << crBBox.right << " " << crBBox.top << " " << kLineToOperator << "\n";
  csAP << crBBox.right << " " << crBBox.bottom << " " << kLineToOperator
       << "\n";
  csAP << crBBox.left << " " << crBBox.bottom << " " << kLineToOperator << "\n";
  csAP << crBBox.left << " " << crBBox.top << " " << kLineToOperator << "\n";

  return ByteString(csAP);
}

ByteString GetAP_Star(const CFX_FloatRect& crBBox) {
  fxcrt::ostringstream csAP;

  float fRadius = (crBBox.top - crBBox.bottom) / (1 + cosf(FXSYS_PI / 5.0f));
  CFX_PointF ptCenter = CFX_PointF((crBBox.left + crBBox.right) / 2.0f,
                                   (crBBox.top + crBBox.bottom) / 2.0f);

  CFX_PointF points[5];
  float fAngle = FXSYS_PI / 10.0f;
  for (auto& point : points) {
    point =
        ptCenter + CFX_PointF(fRadius * cosf(fAngle), fRadius * sinf(fAngle));
    fAngle += FXSYS_PI * 2 / 5.0f;
  }

  csAP << points[0].x << " " << points[0].y << " " << kMoveToOperator << "\n";

  int next = 0;
  for (size_t i = 0; i < pdfium::size(points); ++i) {
    next = (next + 2) % pdfium::size(points);
    csAP << points[next].x << " " << points[next].y << " " << kLineToOperator
         << "\n";
  }

  return ByteString(csAP);
}

ByteString GetAP_HalfCircle(const CFX_FloatRect& crBBox, float fRotate) {
  fxcrt::ostringstream csAP;

  float fWidth = crBBox.Width();
  float fHeight = crBBox.Height();

  CFX_PointF pt1(-fWidth / 2, 0);
  CFX_PointF pt2(0, fHeight / 2);
  CFX_PointF pt3(fWidth / 2, 0);

  float px;
  float py;

  csAP << cos(fRotate) << " " << sin(fRotate) << " " << -sin(fRotate) << " "
       << cos(fRotate) << " " << crBBox.left + fWidth / 2 << " "
       << crBBox.bottom + fHeight / 2 << " " << kConcatMatrixOperator << "\n";

  csAP << pt1.x << " " << pt1.y << " " << kMoveToOperator << "\n";

  px = pt2.x - pt1.x;
  py = pt2.y - pt1.y;

  csAP << pt1.x << " " << pt1.y + py * FXSYS_BEZIER << " "
       << pt2.x - px * FXSYS_BEZIER << " " << pt2.y << " " << pt2.x << " "
       << pt2.y << " " << kCurveToOperator << "\n";

  px = pt3.x - pt2.x;
  py = pt2.y - pt3.y;

  csAP << pt2.x + px * FXSYS_BEZIER << " " << pt2.y << " " << pt3.x << " "
       << pt3.y + py * FXSYS_BEZIER << " " << pt3.x << " " << pt3.y << " "
       << kCurveToOperator << "\n";

  return ByteString(csAP);
}

ByteString GetAppStream_Check(const CFX_FloatRect& rcBBox,
                              const CFX_Color& crText) {
  fxcrt::ostringstream sAP;
  {
    AutoClosedQCommand q(&sAP);
    sAP << GetColorAppStream(crText, true) << GetAP_Check(rcBBox)
        << kFillOperator << "\n";
  }
  return ByteString(sAP);
}

ByteString GetAppStream_Circle(const CFX_FloatRect& rcBBox,
                               const CFX_Color& crText) {
  fxcrt::ostringstream sAP;
  {
    AutoClosedQCommand q(&sAP);
    sAP << GetColorAppStream(crText, true) << GetAP_Circle(rcBBox)
        << kFillOperator << "\n";
  }
  return ByteString(sAP);
}

ByteString GetAppStream_Cross(const CFX_FloatRect& rcBBox,
                              const CFX_Color& crText) {
  fxcrt::ostringstream sAP;
  {
    AutoClosedQCommand q(&sAP);
    sAP << GetColorAppStream(crText, false) << GetAP_Cross(rcBBox)
        << kStrokeOperator << "\n";
  }
  return ByteString(sAP);
}

ByteString GetAppStream_Diamond(const CFX_FloatRect& rcBBox,
                                const CFX_Color& crText) {
  fxcrt::ostringstream sAP;
  {
    AutoClosedQCommand q(&sAP);
    sAP << "1 " << kSetLineWidthOperator << "\n"
        << GetColorAppStream(crText, true) << GetAP_Diamond(rcBBox)
        << kFillOperator << "\n";
  }
  return ByteString(sAP);
}

ByteString GetAppStream_Square(const CFX_FloatRect& rcBBox,
                               const CFX_Color& crText) {
  fxcrt::ostringstream sAP;
  {
    AutoClosedQCommand q(&sAP);
    sAP << GetColorAppStream(crText, true) << GetAP_Square(rcBBox)
        << kFillOperator << "\n";
  }
  return ByteString(sAP);
}

ByteString GetAppStream_Star(const CFX_FloatRect& rcBBox,
                             const CFX_Color& crText) {
  fxcrt::ostringstream sAP;
  {
    AutoClosedQCommand q(&sAP);
    sAP << GetColorAppStream(crText, true) << GetAP_Star(rcBBox)
        << kFillOperator << "\n";
  }
  return ByteString(sAP);
}

ByteString GetCircleFillAppStream(const CFX_FloatRect& rect,
                                  const CFX_Color& color) {
  fxcrt::ostringstream sAppStream;
  ByteString sColor = GetColorAppStream(color, true);
  if (sColor.GetLength() > 0) {
    AutoClosedQCommand q(&sAppStream);
    sAppStream << sColor << GetAP_Circle(rect) << kFillOperator << "\n";
  }
  return ByteString(sAppStream);
}

ByteString GetCircleBorderAppStream(const CFX_FloatRect& rect,
                                    float fWidth,
                                    const CFX_Color& color,
                                    const CFX_Color& crLeftTop,
                                    const CFX_Color& crRightBottom,
                                    BorderStyle nStyle,
                                    const CPWL_Dash& dash) {
  fxcrt::ostringstream sAppStream;
  ByteString sColor;

  if (fWidth > 0.0f) {
    AutoClosedQCommand q(&sAppStream);

    float fHalfWidth = fWidth / 2.0f;
    CFX_FloatRect rect_by_2 = rect.GetDeflated(fHalfWidth, fHalfWidth);

    float div = fHalfWidth * 0.75f;
    CFX_FloatRect rect_by_75 = rect.GetDeflated(div, div);
    switch (nStyle) {
      default:
      case BorderStyle::kSolid:
      case BorderStyle::kUnderline: {
        sColor = GetColorAppStream(color, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor << GetAP_Circle(rect_by_2) << " "
                     << kStrokeOperator << "\n";
        }
      } break;
      case BorderStyle::kDash: {
        sColor = GetColorAppStream(color, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fWidth << " " << kSetLineWidthOperator << "\n"
                     << "[" << dash.nDash << " " << dash.nGap << "] "
                     << dash.nPhase << " " << kSetDashOperator << "\n"
                     << sColor << GetAP_Circle(rect_by_2) << " "
                     << kStrokeOperator << "\n";
        }
      } break;
      case BorderStyle::kBeveled: {
        sColor = GetColorAppStream(color, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fHalfWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor << GetAP_Circle(rect) << " " << kStrokeOperator
                     << "\n";
        }

        sColor = GetColorAppStream(crLeftTop, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fHalfWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor << GetAP_HalfCircle(rect_by_75, FXSYS_PI / 4.0f)
                     << " " << kStrokeOperator << "\n";
        }

        sColor = GetColorAppStream(crRightBottom, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fHalfWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor
                     << GetAP_HalfCircle(rect_by_75, FXSYS_PI * 5 / 4.0f) << " "
                     << kStrokeOperator << "\n";
        }
      } break;
      case BorderStyle::kInset: {
        sColor = GetColorAppStream(color, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fHalfWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor << GetAP_Circle(rect) << " " << kStrokeOperator
                     << "\n";
        }

        sColor = GetColorAppStream(crLeftTop, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fHalfWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor << GetAP_HalfCircle(rect_by_75, FXSYS_PI / 4.0f)
                     << " " << kStrokeOperator << "\n";
        }

        sColor = GetColorAppStream(crRightBottom, false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q2(&sAppStream);
          sAppStream << fHalfWidth << " " << kSetLineWidthOperator << "\n"
                     << sColor
                     << GetAP_HalfCircle(rect_by_75, FXSYS_PI * 5 / 4.0f) << " "
                     << kStrokeOperator << "\n";
        }
      } break;
    }
  }
  return ByteString(sAppStream);
}

ByteString GetCheckBoxAppStream(const CFX_FloatRect& rcBBox,
                                CheckStyle nStyle,
                                const CFX_Color& crText) {
  CFX_FloatRect rcCenter = rcBBox.GetCenterSquare();
  switch (nStyle) {
    default:
    case CheckStyle::kCheck:
      return GetAppStream_Check(rcCenter, crText);
    case CheckStyle::kCircle:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Circle(rcCenter, crText);
    case CheckStyle::kCross:
      return GetAppStream_Cross(rcCenter, crText);
    case CheckStyle::kDiamond:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Diamond(rcCenter, crText);
    case CheckStyle::kSquare:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Square(rcCenter, crText);
    case CheckStyle::kStar:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Star(rcCenter, crText);
  }
}

ByteString GetRadioButtonAppStream(const CFX_FloatRect& rcBBox,
                                   CheckStyle nStyle,
                                   const CFX_Color& crText) {
  CFX_FloatRect rcCenter = rcBBox.GetCenterSquare();
  switch (nStyle) {
    default:
    case CheckStyle::kCheck:
      return GetAppStream_Check(rcCenter, crText);
    case CheckStyle::kCircle:
      rcCenter.ScaleFromCenterPoint(1.0f / 2.0f);
      return GetAppStream_Circle(rcCenter, crText);
    case CheckStyle::kCross:
      return GetAppStream_Cross(rcCenter, crText);
    case CheckStyle::kDiamond:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Diamond(rcCenter, crText);
    case CheckStyle::kSquare:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Square(rcCenter, crText);
    case CheckStyle::kStar:
      rcCenter.ScaleFromCenterPoint(2.0f / 3.0f);
      return GetAppStream_Star(rcCenter, crText);
  }
}

ByteString GetFontSetString(IPVT_FontMap* pFontMap,
                            int32_t nFontIndex,
                            float fFontSize) {
  if (!pFontMap)
    return ByteString();

  ByteString sFontAlias = pFontMap->GetPDFFontAlias(nFontIndex);
  if (sFontAlias.GetLength() <= 0 || fFontSize <= 0)
    return ByteString();

  fxcrt::ostringstream sRet;
  sRet << "/" << sFontAlias << " " << fFontSize << " "
       << kSetTextFontAndSizeOperator << "\n";
  return ByteString(sRet);
}

ByteString GetWordRenderString(ByteStringView strWords) {
  if (strWords.IsEmpty())
    return ByteString();
  return PDF_EncodeString(strWords) + " " + kShowTextOperator + "\n";
}

ByteString GetEditAppStream(CPWL_EditImpl* pEdit,
                            const CFX_PointF& ptOffset,
                            bool bContinuous,
                            uint16_t SubWord) {
  CPWL_EditImpl::Iterator* pIterator = pEdit->GetIterator();
  pIterator->SetAt(0);

  fxcrt::ostringstream sEditStream;
  int32_t nCurFontIndex = -1;
  CFX_PointF ptOld;
  CFX_PointF ptNew;
  CPVT_WordPlace oldplace;
  ByteString sWords;

  while (pIterator->NextWord()) {
    CPVT_WordPlace place = pIterator->GetAt();
    if (bContinuous) {
      if (place.LineCmp(oldplace) != 0) {
        if (!sWords.IsEmpty()) {
          sEditStream << GetWordRenderString(sWords.AsStringView());
          sWords.clear();
        }

        CPVT_Word word;
        if (pIterator->GetWord(word)) {
          ptNew = CFX_PointF(word.ptWord.x + ptOffset.x,
                             word.ptWord.y + ptOffset.y);
        } else {
          CPVT_Line line;
          pIterator->GetLine(line);
          ptNew = CFX_PointF(line.ptLine.x + ptOffset.x,
                             line.ptLine.y + ptOffset.y);
        }

        if (ptNew.x != ptOld.x || ptNew.y != ptOld.y) {
          sEditStream << ptNew.x - ptOld.x << " " << ptNew.y - ptOld.y << " "
                      << kMoveTextPositionOperator << "\n";

          ptOld = ptNew;
        }
      }

      CPVT_Word word;
      if (pIterator->GetWord(word)) {
        if (word.nFontIndex != nCurFontIndex) {
          if (!sWords.IsEmpty()) {
            sEditStream << GetWordRenderString(sWords.AsStringView());
            sWords.clear();
          }
          sEditStream << GetFontSetString(pEdit->GetFontMap(), word.nFontIndex,
                                          word.fFontSize);
          nCurFontIndex = word.nFontIndex;
        }

        sWords += pEdit->GetPDFWordString(nCurFontIndex, word.Word, SubWord);
      }
      oldplace = place;
    } else {
      CPVT_Word word;
      if (pIterator->GetWord(word)) {
        ptNew =
            CFX_PointF(word.ptWord.x + ptOffset.x, word.ptWord.y + ptOffset.y);

        if (ptNew.x != ptOld.x || ptNew.y != ptOld.y) {
          sEditStream << ptNew.x - ptOld.x << " " << ptNew.y - ptOld.y << " "
                      << kMoveTextPositionOperator << "\n";
          ptOld = ptNew;
        }
        if (word.nFontIndex != nCurFontIndex) {
          sEditStream << GetFontSetString(pEdit->GetFontMap(), word.nFontIndex,
                                          word.fFontSize);
          nCurFontIndex = word.nFontIndex;
        }
        sEditStream << GetWordRenderString(
            pEdit->GetPDFWordString(nCurFontIndex, word.Word, SubWord)
                .AsStringView());
      }
    }
  }

  if (!sWords.IsEmpty())
    sEditStream << GetWordRenderString(sWords.AsStringView());

  fxcrt::ostringstream sAppStream;
  if (sEditStream.tellp() > 0) {
    sAppStream << sEditStream.str();
  }
  return ByteString(sAppStream);
}

ByteString GenerateIconAppStream(CPDF_IconFit& fit,
                                 CPDF_Stream* pIconStream,
                                 const CFX_FloatRect& rcIcon) {
  if (rcIcon.IsEmpty() || !pIconStream)
    return ByteString();

  auto pPDFIcon = std::make_unique<CPDF_Icon>(pIconStream);

  CPWL_Wnd::CreateParams cp;
  cp.dwFlags = PWS_VISIBLE;
  auto pWnd = std::make_unique<CPWL_Wnd>(cp, nullptr);
  pWnd->Realize();
  if (!pWnd->Move(rcIcon, false, false))
    return ByteString();

  ByteString sAlias = pPDFIcon->GetImageAlias();
  if (sAlias.GetLength() <= 0)
    return ByteString();

  const CFX_FloatRect rcPlate = pWnd->GetClientRect();
  const CFX_SizeF image_size = pPDFIcon->GetImageSize();
  const CFX_Matrix mt = pPDFIcon->GetImageMatrix().GetInverse();
  const CFX_VectorF scale = fit.GetScale(image_size, rcPlate);
  const CFX_VectorF offset = fit.GetImageOffset(image_size, scale, rcPlate);

  fxcrt::ostringstream str;
  {
    AutoClosedQCommand q(&str);
    str << rcPlate.left << " " << rcPlate.bottom << " "
        << rcPlate.right - rcPlate.left << " " << rcPlate.top - rcPlate.bottom
        << " " << kAppendRectOperator << " " << kSetNonZeroWindingClipOperator
        << " " << kEndPathNoFillOrStrokeOperator << "\n";

    str << scale.x << " 0 0 " << scale.y << " " << rcPlate.left + offset.x
        << " " << rcPlate.bottom + offset.y << " " << kConcatMatrixOperator
        << "\n";
    str << mt.a << " " << mt.b << " " << mt.c << " " << mt.d << " " << mt.e
        << " " << mt.f << " " << kConcatMatrixOperator << "\n";

    str << "0 " << kSetGrayOperator << " 0 " << kSetGrayStrokedOperator << " 1 "
        << kSetLineWidthOperator << " /" << sAlias << " "
        << kInvokeNamedXObjectOperator << "\n";
  }
  pWnd->Destroy();
  return ByteString(str);
}

ByteString GetPushButtonAppStream(const CFX_FloatRect& rcBBox,
                                  IPVT_FontMap* pFontMap,
                                  CPDF_Stream* pIconStream,
                                  CPDF_IconFit& IconFit,
                                  const WideString& sLabel,
                                  const CFX_Color& crText,
                                  float fFontSize,
                                  ButtonStyle nLayOut) {
  const float fAutoFontScale = 1.0f / 3.0f;

  auto pEdit = std::make_unique<CPWL_EditImpl>();
  pEdit->SetFontMap(pFontMap);
  pEdit->SetAlignmentH(1);
  pEdit->SetAlignmentV(1);
  pEdit->SetMultiLine(false);
  pEdit->SetAutoReturn(false);
  if (FXSYS_IsFloatZero(fFontSize))
    pEdit->SetAutoFontSize(true);
  else
    pEdit->SetFontSize(fFontSize);

  pEdit->Initialize();
  pEdit->SetText(sLabel);
  pEdit->Paint();

  CFX_FloatRect rcLabelContent = pEdit->GetContentRect();
  CFX_FloatRect rcLabel;
  CFX_FloatRect rcIcon;
  float fWidth = 0.0f;
  float fHeight = 0.0f;

  switch (nLayOut) {
    case ButtonStyle::kLabel:
      rcLabel = rcBBox;
      break;
    case ButtonStyle::kIcon:
      rcIcon = rcBBox;
      break;
    case ButtonStyle::kIconTopLabelBottom:
      if (pIconStream) {
        if (FXSYS_IsFloatZero(fFontSize)) {
          fHeight = rcBBox.Height();
          rcLabel = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcBBox.right,
                                  rcBBox.bottom + fHeight * fAutoFontScale);
          rcIcon =
              CFX_FloatRect(rcBBox.left, rcLabel.top, rcBBox.right, rcBBox.top);
        } else {
          fHeight = rcLabelContent.Height();

          if (rcBBox.bottom + fHeight > rcBBox.top) {
            rcLabel = rcBBox;
          } else {
            rcLabel = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcBBox.right,
                                    rcBBox.bottom + fHeight);
            rcIcon = CFX_FloatRect(rcBBox.left, rcLabel.top, rcBBox.right,
                                   rcBBox.top);
          }
        }
      } else {
        rcLabel = rcBBox;
      }
      break;
    case ButtonStyle::kIconBottomLabelTop:
      if (pIconStream) {
        if (FXSYS_IsFloatZero(fFontSize)) {
          fHeight = rcBBox.Height();
          rcLabel =
              CFX_FloatRect(rcBBox.left, rcBBox.top - fHeight * fAutoFontScale,
                            rcBBox.right, rcBBox.top);
          rcIcon = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcBBox.right,
                                 rcLabel.bottom);
        } else {
          fHeight = rcLabelContent.Height();

          if (rcBBox.bottom + fHeight > rcBBox.top) {
            rcLabel = rcBBox;
          } else {
            rcLabel = CFX_FloatRect(rcBBox.left, rcBBox.top - fHeight,
                                    rcBBox.right, rcBBox.top);
            rcIcon = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcBBox.right,
                                   rcLabel.bottom);
          }
        }
      } else {
        rcLabel = rcBBox;
      }
      break;
    case ButtonStyle::kIconLeftLabelRight:
      if (pIconStream) {
        if (FXSYS_IsFloatZero(fFontSize)) {
          fWidth = rcBBox.right - rcBBox.left;
          if (rcLabelContent.Width() < fWidth * fAutoFontScale) {
            rcLabel = CFX_FloatRect(rcBBox.right - fWidth * fAutoFontScale,
                                    rcBBox.bottom, rcBBox.right, rcBBox.top);
            rcIcon = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcLabel.left,
                                   rcBBox.top);
          } else {
            if (rcLabelContent.Width() < fWidth) {
              rcLabel = CFX_FloatRect(rcBBox.right - rcLabelContent.Width(),
                                      rcBBox.bottom, rcBBox.right, rcBBox.top);
              rcIcon = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcLabel.left,
                                     rcBBox.top);
            } else {
              rcLabel = rcBBox;
            }
          }
        } else {
          fWidth = rcLabelContent.Width();
          if (rcBBox.left + fWidth > rcBBox.right) {
            rcLabel = rcBBox;
          } else {
            rcLabel = CFX_FloatRect(rcBBox.right - fWidth, rcBBox.bottom,
                                    rcBBox.right, rcBBox.top);
            rcIcon = CFX_FloatRect(rcBBox.left, rcBBox.bottom, rcLabel.left,
                                   rcBBox.top);
          }
        }
      } else {
        rcLabel = rcBBox;
      }
      break;
    case ButtonStyle::kIconRightLabelLeft:
      if (pIconStream) {
        if (FXSYS_IsFloatZero(fFontSize)) {
          fWidth = rcBBox.right - rcBBox.left;
          if (rcLabelContent.Width() < fWidth * fAutoFontScale) {
            rcLabel = CFX_FloatRect(rcBBox.left, rcBBox.bottom,
                                    rcBBox.left + fWidth * fAutoFontScale,
                                    rcBBox.top);
            rcIcon = CFX_FloatRect(rcLabel.right, rcBBox.bottom, rcBBox.right,
                                   rcBBox.top);
          } else {
            if (rcLabelContent.Width() < fWidth) {
              rcLabel = CFX_FloatRect(rcBBox.left, rcBBox.bottom,
                                      rcBBox.left + rcLabelContent.Width(),
                                      rcBBox.top);
              rcIcon = CFX_FloatRect(rcLabel.right, rcBBox.bottom, rcBBox.right,
                                     rcBBox.top);
            } else {
              rcLabel = rcBBox;
            }
          }
        } else {
          fWidth = rcLabelContent.Width();
          if (rcBBox.left + fWidth > rcBBox.right) {
            rcLabel = rcBBox;
          } else {
            rcLabel = CFX_FloatRect(rcBBox.left, rcBBox.bottom,
                                    rcBBox.left + fWidth, rcBBox.top);
            rcIcon = CFX_FloatRect(rcLabel.right, rcBBox.bottom, rcBBox.right,
                                   rcBBox.top);
          }
        }
      } else {
        rcLabel = rcBBox;
      }
      break;
    case ButtonStyle::kLabelOverIcon:
      rcLabel = rcBBox;
      rcIcon = rcBBox;
      break;
  }

  fxcrt::ostringstream sTemp;
  sTemp << GenerateIconAppStream(IconFit, pIconStream, rcIcon);

  if (!rcLabel.IsEmpty()) {
    pEdit->SetPlateRect(rcLabel);
    pEdit->Paint();
    ByteString sEdit =
        GetEditAppStream(pEdit.get(), CFX_PointF(0.0f, 0.0f), true, 0);
    if (sEdit.GetLength() > 0) {
      AutoClosedCommand bt(&sTemp, kTextBeginOperator, kTextEndOperator);
      sTemp << GetColorAppStream(crText, true) << sEdit;
    }
  }

  if (sTemp.tellp() <= 0)
    return ByteString();

  fxcrt::ostringstream sAppStream;
  {
    AutoClosedQCommand q(&sAppStream);
    sAppStream << rcBBox.left << " " << rcBBox.bottom << " "
               << rcBBox.right - rcBBox.left << " "
               << rcBBox.top - rcBBox.bottom << " " << kAppendRectOperator
               << " " << kSetNonZeroWindingClipOperator << " "
               << kEndPathNoFillOrStrokeOperator << "\n";
    sAppStream << sTemp.str().c_str();
  }
  return ByteString(sAppStream);
}

ByteString GetBorderAppStreamInternal(const CFX_FloatRect& rect,
                                      float fWidth,
                                      const CFX_Color& color,
                                      const CFX_Color& crLeftTop,
                                      const CFX_Color& crRightBottom,
                                      BorderStyle nStyle,
                                      const CPWL_Dash& dash) {
  fxcrt::ostringstream sAppStream;
  ByteString sColor;

  float fLeft = rect.left;
  float fRight = rect.right;
  float fTop = rect.top;
  float fBottom = rect.bottom;

  if (fWidth > 0.0f) {
    float fHalfWidth = fWidth / 2.0f;
    AutoClosedQCommand q(&sAppStream);

    switch (nStyle) {
      default:
      case BorderStyle::kSolid:
        sColor = GetColorAppStream(color, true);
        if (sColor.GetLength() > 0) {
          sAppStream << sColor;
          sAppStream << fLeft << " " << fBottom << " " << fRight - fLeft << " "
                     << fTop - fBottom << " " << kAppendRectOperator << "\n";
          sAppStream << fLeft + fWidth << " " << fBottom + fWidth << " "
                     << fRight - fLeft - fWidth * 2 << " "
                     << fTop - fBottom - fWidth * 2 << " "
                     << kAppendRectOperator << "\n";
          sAppStream << kFillEvenOddOperator << "\n";
        }
        break;
      case BorderStyle::kDash:
        sColor = GetColorAppStream(color, false);
        if (sColor.GetLength() > 0) {
          sAppStream << sColor;
          sAppStream << fWidth << " " << kSetLineWidthOperator << " ["
                     << dash.nDash << " " << dash.nGap << "] " << dash.nPhase
                     << " " << kSetDashOperator << "\n";
          sAppStream << fLeft + fWidth / 2 << " " << fBottom + fWidth / 2 << " "
                     << kMoveToOperator << "\n";
          sAppStream << fLeft + fWidth / 2 << " " << fTop - fWidth / 2 << " "
                     << kLineToOperator << "\n";
          sAppStream << fRight - fWidth / 2 << " " << fTop - fWidth / 2 << " "
                     << kLineToOperator << "\n";
          sAppStream << fRight - fWidth / 2 << " " << fBottom + fWidth / 2
                     << " " << kLineToOperator << "\n";
          sAppStream << fLeft + fWidth / 2 << " " << fBottom + fWidth / 2 << " "
                     << kLineToOperator << " " << kStrokeOperator << "\n";
        }
        break;
      case BorderStyle::kBeveled:
      case BorderStyle::kInset:
        sColor = GetColorAppStream(crLeftTop, true);
        if (sColor.GetLength() > 0) {
          sAppStream << sColor;
          sAppStream << fLeft + fHalfWidth << " " << fBottom + fHalfWidth << " "
                     << kMoveToOperator << "\n";
          sAppStream << fLeft + fHalfWidth << " " << fTop - fHalfWidth << " "
                     << kLineToOperator << "\n";
          sAppStream << fRight - fHalfWidth << " " << fTop - fHalfWidth << " "
                     << kLineToOperator << "\n";
          sAppStream << fRight - fHalfWidth * 2 << " " << fTop - fHalfWidth * 2
                     << " " << kLineToOperator << "\n";
          sAppStream << fLeft + fHalfWidth * 2 << " " << fTop - fHalfWidth * 2
                     << " " << kLineToOperator << "\n";
          sAppStream << fLeft + fHalfWidth * 2 << " "
                     << fBottom + fHalfWidth * 2 << " " << kLineToOperator
                     << " " << kFillOperator << "\n";
        }

        sColor = GetColorAppStream(crRightBottom, true);
        if (sColor.GetLength() > 0) {
          sAppStream << sColor;
          sAppStream << fRight - fHalfWidth << " " << fTop - fHalfWidth << " "
                     << kMoveToOperator << "\n";
          sAppStream << fRight - fHalfWidth << " " << fBottom + fHalfWidth
                     << " " << kLineToOperator << "\n";
          sAppStream << fLeft + fHalfWidth << " " << fBottom + fHalfWidth << " "
                     << kLineToOperator << "\n";
          sAppStream << fLeft + fHalfWidth * 2 << " "
                     << fBottom + fHalfWidth * 2 << " " << kLineToOperator
                     << "\n";
          sAppStream << fRight - fHalfWidth * 2 << " "
                     << fBottom + fHalfWidth * 2 << " " << kLineToOperator
                     << "\n";
          sAppStream << fRight - fHalfWidth * 2 << " " << fTop - fHalfWidth * 2
                     << " " << kLineToOperator << " " << kFillOperator << "\n";
        }

        sColor = GetColorAppStream(color, true);
        if (sColor.GetLength() > 0) {
          sAppStream << sColor;
          sAppStream << fLeft << " " << fBottom << " " << fRight - fLeft << " "
                     << fTop - fBottom << " " << kAppendRectOperator << "\n";
          sAppStream << fLeft + fHalfWidth << " " << fBottom + fHalfWidth << " "
                     << fRight - fLeft - fHalfWidth * 2 << " "
                     << fTop - fBottom - fHalfWidth * 2 << " "
                     << kAppendRectOperator << " " << kFillEvenOddOperator
                     << "\n";
        }
        break;
      case BorderStyle::kUnderline:
        sColor = GetColorAppStream(color, false);
        if (sColor.GetLength() > 0) {
          sAppStream << sColor;
          sAppStream << fWidth << " " << kSetLineWidthOperator << "\n";
          sAppStream << fLeft << " " << fBottom + fWidth / 2 << " "
                     << kMoveToOperator << "\n";
          sAppStream << fRight << " " << fBottom + fWidth / 2 << " "
                     << kLineToOperator << " " << kStrokeOperator << "\n";
        }
        break;
    }
  }

  return ByteString(sAppStream);
}

ByteString GetDropButtonAppStream(const CFX_FloatRect& rcBBox) {
  if (rcBBox.IsEmpty())
    return ByteString();

  fxcrt::ostringstream sAppStream;
  {
    AutoClosedQCommand q(&sAppStream);
    sAppStream << GetColorAppStream(
                      CFX_Color(CFX_Color::Type::kRGB, 220.0f / 255.0f,
                                220.0f / 255.0f, 220.0f / 255.0f),
                      true)
               << rcBBox.left << " " << rcBBox.bottom << " "
               << rcBBox.right - rcBBox.left << " "
               << rcBBox.top - rcBBox.bottom << " " << kAppendRectOperator
               << " " << kFillOperator << "\n";
  }

  {
    AutoClosedQCommand q(&sAppStream);
    sAppStream << GetBorderAppStreamInternal(
        rcBBox, 2, CFX_Color(CFX_Color::Type::kGray, 0),
        CFX_Color(CFX_Color::Type::kGray, 1),
        CFX_Color(CFX_Color::Type::kGray, 0.5), BorderStyle::kBeveled,
        CPWL_Dash(3, 0, 0));
  }

  CFX_PointF ptCenter = CFX_PointF((rcBBox.left + rcBBox.right) / 2,
                                   (rcBBox.top + rcBBox.bottom) / 2);
  if (FXSYS_IsFloatBigger(rcBBox.right - rcBBox.left, 6) &&
      FXSYS_IsFloatBigger(rcBBox.top - rcBBox.bottom, 6)) {
    AutoClosedQCommand q(&sAppStream);
    sAppStream << " 0 " << kSetGrayOperator << "\n"
               << ptCenter.x - 3 << " " << ptCenter.y + 1.5f << " "
               << kMoveToOperator << "\n"
               << ptCenter.x + 3 << " " << ptCenter.y + 1.5f << " "
               << kLineToOperator << "\n"
               << ptCenter.x << " " << ptCenter.y - 1.5f << " "
               << kLineToOperator << "\n"
               << ptCenter.x - 3 << " " << ptCenter.y + 1.5f << " "
               << kLineToOperator << " " << kFillOperator << "\n";
  }

  return ByteString(sAppStream);
}

ByteString GetRectFillAppStream(const CFX_FloatRect& rect,
                                const CFX_Color& color) {
  fxcrt::ostringstream sAppStream;
  ByteString sColor = GetColorAppStream(color, true);
  if (sColor.GetLength() > 0) {
    AutoClosedQCommand q(&sAppStream);
    sAppStream << sColor << rect.left << " " << rect.bottom << " "
               << rect.right - rect.left << " " << rect.top - rect.bottom << " "
               << kAppendRectOperator << " " << kFillOperator << "\n";
  }

  return ByteString(sAppStream);
}

void SetDefaultIconName(CPDF_Stream* pIcon, const char* name) {
  if (!pIcon)
    return;

  CPDF_Dictionary* pImageDict = pIcon->GetDict();
  if (!pImageDict)
    return;

  if (pImageDict->KeyExist("Name"))
    return;

  pImageDict->SetNewFor<CPDF_String>("Name", name, false);
}

absl::optional<CheckStyle> CheckStyleFromCaption(const WideString& caption) {
  if (caption.IsEmpty())
    return absl::nullopt;

  // Character values are ZapfDingbats encodings of named glyphs.
  switch (caption[0]) {
    case L'4':
      return CheckStyle::kCheck;
    case L'8':
      return CheckStyle::kCross;
    case L'H':
      return CheckStyle::kStar;
    case L'l':
      return CheckStyle::kCircle;
    case L'n':
      return CheckStyle::kSquare;
    case L'u':
      return CheckStyle::kDiamond;
    default:
      return absl::nullopt;
  }
}

}  // namespace

CPDFSDK_AppStream::CPDFSDK_AppStream(CPDFSDK_Widget* widget,
                                     CPDF_Dictionary* dict)
    : widget_(widget), dict_(dict) {}

CPDFSDK_AppStream::~CPDFSDK_AppStream() = default;

void CPDFSDK_AppStream::SetAsPushButton() {
  CPDF_FormControl* pControl = widget_->GetFormControl();
  CFX_FloatRect rcWindow = widget_->GetRotatedRect();
  ButtonStyle nLayout = ButtonStyle::kLabel;
  switch (pControl->GetTextPosition()) {
    case TEXTPOS_ICON:
      nLayout = ButtonStyle::kIcon;
      break;
    case TEXTPOS_BELOW:
      nLayout = ButtonStyle::kIconTopLabelBottom;
      break;
    case TEXTPOS_ABOVE:
      nLayout = ButtonStyle::kIconBottomLabelTop;
      break;
    case TEXTPOS_RIGHT:
      nLayout = ButtonStyle::kIconLeftLabelRight;
      break;
    case TEXTPOS_LEFT:
      nLayout = ButtonStyle::kIconRightLabelLeft;
      break;
    case TEXTPOS_OVERLAID:
      nLayout = ButtonStyle::kLabelOverIcon;
      break;
    default:
      nLayout = ButtonStyle::kLabel;
      break;
  }

  CFX_Color crBackground = pControl->GetOriginalBackgroundColor();
  CFX_Color crBorder = pControl->GetOriginalBorderColor();

  float fBorderWidth = static_cast<float>(widget_->GetBorderWidth());
  CPWL_Dash dsBorder(3, 0, 0);
  CFX_Color crLeftTop;
  CFX_Color crRightBottom;

  BorderStyle nBorderStyle = widget_->GetBorderStyle();
  switch (nBorderStyle) {
    case BorderStyle::kDash:
      dsBorder = CPWL_Dash(3, 3, 0);
      break;
    case BorderStyle::kBeveled:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 1);
      crRightBottom = crBackground / 2.0f;
      break;
    case BorderStyle::kInset:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0.5);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 0.75);
      break;
    default:
      break;
  }

  CFX_FloatRect rcClient = rcWindow.GetDeflated(fBorderWidth, fBorderWidth);
  CPDF_DefaultAppearance da = pControl->GetDefaultAppearance();
  absl::optional<CFX_Color> color = da.GetColor();
  CFX_Color crText = color.value_or(CFX_Color(CFX_Color::Type::kGray, 0));

  float fFontSize;
  ByteString csNameTag;
  absl::optional<ByteString> font = da.GetFont(&fFontSize);
  if (font.has_value())
    csNameTag = font.value();
  else
    fFontSize = 12.0f;

  WideString csWCaption;
  WideString csNormalCaption;
  WideString csRolloverCaption;
  WideString csDownCaption;
  if (pControl->HasMKEntry(pdfium::appearance::kCA))
    csNormalCaption = pControl->GetNormalCaption();

  if (pControl->HasMKEntry(pdfium::appearance::kRC))
    csRolloverCaption = pControl->GetRolloverCaption();

  if (pControl->HasMKEntry(pdfium::appearance::kAC))
    csDownCaption = pControl->GetDownCaption();

  CPDF_Stream* pNormalIcon = nullptr;
  CPDF_Stream* pRolloverIcon = nullptr;
  CPDF_Stream* pDownIcon = nullptr;
  if (pControl->HasMKEntry(pdfium::appearance::kI))
    pNormalIcon = pControl->GetNormalIcon();

  if (pControl->HasMKEntry(pdfium::appearance::kRI))
    pRolloverIcon = pControl->GetRolloverIcon();

  if (pControl->HasMKEntry(pdfium::appearance::kIX))
    pDownIcon = pControl->GetDownIcon();

  SetDefaultIconName(pNormalIcon, "ImgA");
  SetDefaultIconName(pRolloverIcon, "ImgB");
  SetDefaultIconName(pDownIcon, "ImgC");

  CPDF_IconFit iconFit = pControl->GetIconFit();
  {
    CPDF_BAFontMap font_map(widget_->GetPDFPage()->GetDocument(),
                            widget_->GetPDFAnnot()->GetAnnotDict(), "N");
    ByteString csAP =
        GetRectFillAppStream(rcWindow, crBackground) +
        GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                   crRightBottom, nBorderStyle, dsBorder) +
        GetPushButtonAppStream(iconFit.GetFittingBounds() ? rcWindow : rcClient,
                               &font_map, pNormalIcon, iconFit, csNormalCaption,
                               crText, fFontSize, nLayout);

    Write("N", csAP, ByteString());
    if (pNormalIcon)
      AddImage("N", pNormalIcon);

    CPDF_FormControl::HighlightingMode eHLM = pControl->GetHighlightingMode();
    if (eHLM != CPDF_FormControl::kPush && eHLM != CPDF_FormControl::kToggle) {
      Remove("D");
      Remove("R");
      return;
    }

    if (csRolloverCaption.IsEmpty() && !pRolloverIcon) {
      csRolloverCaption = csNormalCaption;
      pRolloverIcon = pNormalIcon;
    }
  }
  {
    CPDF_BAFontMap font_map(widget_->GetPDFPage()->GetDocument(),
                            widget_->GetPDFAnnot()->GetAnnotDict(), "R");
    ByteString csAP =
        GetRectFillAppStream(rcWindow, crBackground) +
        GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                   crRightBottom, nBorderStyle, dsBorder) +
        GetPushButtonAppStream(iconFit.GetFittingBounds() ? rcWindow : rcClient,
                               &font_map, pRolloverIcon, iconFit,
                               csRolloverCaption, crText, fFontSize, nLayout);

    Write("R", csAP, ByteString());
    if (pRolloverIcon)
      AddImage("R", pRolloverIcon);

    if (csDownCaption.IsEmpty() && !pDownIcon) {
      csDownCaption = csNormalCaption;
      pDownIcon = pNormalIcon;
    }

    switch (nBorderStyle) {
      case BorderStyle::kBeveled: {
        CFX_Color crTemp = crLeftTop;
        crLeftTop = crRightBottom;
        crRightBottom = crTemp;
        break;
      }
      case BorderStyle::kInset: {
        crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0);
        crRightBottom = CFX_Color(CFX_Color::Type::kGray, 1);
        break;
      }
      default:
        break;
    }
  }
  {
    CPDF_BAFontMap font_map(widget_->GetPDFPage()->GetDocument(),
                            widget_->GetPDFAnnot()->GetAnnotDict(), "D");
    ByteString csAP =
        GetRectFillAppStream(rcWindow, crBackground - 0.25f) +
        GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                   crRightBottom, nBorderStyle, dsBorder) +
        GetPushButtonAppStream(iconFit.GetFittingBounds() ? rcWindow : rcClient,
                               &font_map, pDownIcon, iconFit, csDownCaption,
                               crText, fFontSize, nLayout);

    Write("D", csAP, ByteString());
    if (pDownIcon)
      AddImage("D", pDownIcon);
  }
}

void CPDFSDK_AppStream::SetAsCheckBox() {
  CPDF_FormControl* pControl = widget_->GetFormControl();
  CFX_Color crBackground = pControl->GetOriginalBackgroundColor();
  CFX_Color crBorder = pControl->GetOriginalBorderColor();
  float fBorderWidth = static_cast<float>(widget_->GetBorderWidth());
  CPWL_Dash dsBorder(3, 0, 0);
  CFX_Color crLeftTop;
  CFX_Color crRightBottom;

  BorderStyle nBorderStyle = widget_->GetBorderStyle();
  switch (nBorderStyle) {
    case BorderStyle::kDash:
      dsBorder = CPWL_Dash(3, 3, 0);
      break;
    case BorderStyle::kBeveled:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 1);
      crRightBottom = crBackground / 2.0f;
      break;
    case BorderStyle::kInset:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0.5);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 0.75);
      break;
    default:
      break;
  }

  CFX_FloatRect rcWindow = widget_->GetRotatedRect();
  CFX_FloatRect rcClient = rcWindow.GetDeflated(fBorderWidth, fBorderWidth);
  absl::optional<CFX_Color> color = pControl->GetDefaultAppearance().GetColor();
  CFX_Color crText = color.value_or(CFX_Color());

  CheckStyle nStyle = CheckStyleFromCaption(pControl->GetNormalCaption())
                          .value_or(CheckStyle::kCheck);
  ByteString csAP_N_ON =
      GetRectFillAppStream(rcWindow, crBackground) +
      GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                 crRightBottom, nBorderStyle, dsBorder);

  ByteString csAP_N_OFF = csAP_N_ON;

  switch (nBorderStyle) {
    case BorderStyle::kBeveled: {
      CFX_Color crTemp = crLeftTop;
      crLeftTop = crRightBottom;
      crRightBottom = crTemp;
      break;
    }
    case BorderStyle::kInset: {
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 1);
      break;
    }
    default:
      break;
  }

  ByteString csAP_D_ON =
      GetRectFillAppStream(rcWindow, crBackground - 0.25f) +
      GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                 crRightBottom, nBorderStyle, dsBorder);

  ByteString csAP_D_OFF = csAP_D_ON;

  csAP_N_ON += GetCheckBoxAppStream(rcClient, nStyle, crText);
  csAP_D_ON += GetCheckBoxAppStream(rcClient, nStyle, crText);

  Write("N", csAP_N_ON, pControl->GetCheckedAPState());
  Write("N", csAP_N_OFF, "Off");

  Write("D", csAP_D_ON, pControl->GetCheckedAPState());
  Write("D", csAP_D_OFF, "Off");

  ByteString csAS = widget_->GetAppState();
  if (csAS.IsEmpty())
    widget_->SetAppStateOff();
}

void CPDFSDK_AppStream::SetAsRadioButton() {
  CPDF_FormControl* pControl = widget_->GetFormControl();
  CFX_Color crBackground = pControl->GetOriginalBackgroundColor();
  CFX_Color crBorder = pControl->GetOriginalBorderColor();
  float fBorderWidth = static_cast<float>(widget_->GetBorderWidth());
  CPWL_Dash dsBorder(3, 0, 0);
  CFX_Color crLeftTop;
  CFX_Color crRightBottom;

  BorderStyle nBorderStyle = widget_->GetBorderStyle();
  switch (nBorderStyle) {
    case BorderStyle::kDash:
      dsBorder = CPWL_Dash(3, 3, 0);
      break;
    case BorderStyle::kBeveled:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 1);
      crRightBottom = crBackground / 2.0f;
      break;
    case BorderStyle::kInset:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0.5);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 0.75);
      break;
    default:
      break;
  }

  CFX_FloatRect rcWindow = widget_->GetRotatedRect();
  CFX_FloatRect rcClient = rcWindow.GetDeflated(fBorderWidth, fBorderWidth);
  absl::optional<CFX_Color> color = pControl->GetDefaultAppearance().GetColor();
  CFX_Color crText = color.value_or(CFX_Color());
  CheckStyle nStyle = CheckStyleFromCaption(pControl->GetNormalCaption())
                          .value_or(CheckStyle::kCircle);

  ByteString csAP_N_ON;
  CFX_FloatRect rcCenter = rcWindow.GetCenterSquare().GetDeflated(1.0f, 1.0f);
  if (nStyle == CheckStyle::kCircle) {
    if (nBorderStyle == BorderStyle::kBeveled) {
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 1);
      crRightBottom = crBackground - 0.25f;
    } else if (nBorderStyle == BorderStyle::kInset) {
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0.5f);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 0.75f);
    }

    csAP_N_ON =
        GetCircleFillAppStream(rcCenter, crBackground) +
        GetCircleBorderAppStream(rcCenter, fBorderWidth, crBorder, crLeftTop,
                                 crRightBottom, nBorderStyle, dsBorder);
  } else {
    csAP_N_ON =
        GetRectFillAppStream(rcWindow, crBackground) +
        GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                   crRightBottom, nBorderStyle, dsBorder);
  }

  ByteString csAP_N_OFF = csAP_N_ON;

  switch (nBorderStyle) {
    case BorderStyle::kBeveled: {
      CFX_Color crTemp = crLeftTop;
      crLeftTop = crRightBottom;
      crRightBottom = crTemp;
      break;
    }
    case BorderStyle::kInset: {
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 1);
      break;
    }
    default:
      break;
  }

  ByteString csAP_D_ON;

  if (nStyle == CheckStyle::kCircle) {
    CFX_Color crBK = crBackground - 0.25f;
    if (nBorderStyle == BorderStyle::kBeveled) {
      crLeftTop = crBackground - 0.25f;
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 1);
      crBK = crBackground;
    } else if (nBorderStyle == BorderStyle::kInset) {
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 1);
    }

    csAP_D_ON =
        GetCircleFillAppStream(rcCenter, crBK) +
        GetCircleBorderAppStream(rcCenter, fBorderWidth, crBorder, crLeftTop,
                                 crRightBottom, nBorderStyle, dsBorder);
  } else {
    csAP_D_ON =
        GetRectFillAppStream(rcWindow, crBackground - 0.25f) +
        GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                   crRightBottom, nBorderStyle, dsBorder);
  }

  ByteString csAP_D_OFF = csAP_D_ON;

  ByteString app_stream = GetRadioButtonAppStream(rcClient, nStyle, crText);
  csAP_N_ON += app_stream;
  csAP_D_ON += app_stream;

  Write("N", csAP_N_ON, pControl->GetCheckedAPState());
  Write("N", csAP_N_OFF, "Off");

  Write("D", csAP_D_ON, pControl->GetCheckedAPState());
  Write("D", csAP_D_OFF, "Off");

  ByteString csAS = widget_->GetAppState();
  if (csAS.IsEmpty())
    widget_->SetAppStateOff();
}

void CPDFSDK_AppStream::SetAsComboBox(absl::optional<WideString> sValue) {
  CPDF_FormControl* pControl = widget_->GetFormControl();
  CPDF_FormField* pField = pControl->GetField();
  fxcrt::ostringstream sBody;

  CFX_FloatRect rcClient = widget_->GetClientRect();
  CFX_FloatRect rcButton = rcClient;
  rcButton.left = rcButton.right - 13;
  rcButton.Normalize();

  // Font map must outlive |pEdit|.
  CPDF_BAFontMap font_map(widget_->GetPDFPage()->GetDocument(),
                          widget_->GetPDFAnnot()->GetAnnotDict(), "N");

  auto pEdit = std::make_unique<CPWL_EditImpl>();
  pEdit->EnableRefresh(false);
  pEdit->SetFontMap(&font_map);

  CFX_FloatRect rcEdit = rcClient;
  rcEdit.right = rcButton.left;
  rcEdit.Normalize();

  pEdit->SetPlateRect(rcEdit);
  pEdit->SetAlignmentV(1);

  float fFontSize = widget_->GetFontSize();
  if (FXSYS_IsFloatZero(fFontSize))
    pEdit->SetAutoFontSize(true);
  else
    pEdit->SetFontSize(fFontSize);

  pEdit->Initialize();
  if (sValue.has_value()) {
    pEdit->SetText(sValue.value());
  } else {
    int32_t nCurSel = pField->GetSelectedIndex(0);
    if (nCurSel < 0) {
      pEdit->SetText(pField->GetValue());
    } else {
      pEdit->SetText(pField->GetOptionLabel(nCurSel));
    }
  }
  pEdit->Paint();

  CFX_FloatRect rcContent = pEdit->GetContentRect();
  ByteString sEdit = GetEditAppStream(pEdit.get(), CFX_PointF(), true, 0);
  if (sEdit.GetLength() > 0) {
    sBody << "/Tx ";
    AutoClosedCommand bmc(&sBody, kMarkedSequenceBeginOperator,
                          kMarkedSequenceEndOperator);
    AutoClosedQCommand q(&sBody);

    if (rcContent.Width() > rcEdit.Width() ||
        rcContent.Height() > rcEdit.Height()) {
      sBody << rcEdit.left << " " << rcEdit.bottom << " " << rcEdit.Width()
            << " " << rcEdit.Height() << " " << kAppendRectOperator << "\n"
            << kSetNonZeroWindingClipOperator << "\n"
            << kEndPathNoFillOrStrokeOperator << "\n";
    }

    CFX_Color crText = widget_->GetTextPWLColor();
    AutoClosedCommand bt(&sBody, kTextBeginOperator, kTextEndOperator);
    sBody << GetColorAppStream(crText, true) << sEdit;
  }

  sBody << GetDropButtonAppStream(rcButton);
  Write("N",
        GetBackgroundAppStream() + GetBorderAppStream() + ByteString(sBody),
        ByteString());
}

void CPDFSDK_AppStream::SetAsListBox() {
  CPDF_FormControl* pControl = widget_->GetFormControl();
  CPDF_FormField* pField = pControl->GetField();
  CFX_FloatRect rcClient = widget_->GetClientRect();
  fxcrt::ostringstream sBody;

  // Font map must outlive |pEdit|.
  CPDF_BAFontMap font_map(widget_->GetPDFPage()->GetDocument(),
                          widget_->GetPDFAnnot()->GetAnnotDict(), "N");

  auto pEdit = std::make_unique<CPWL_EditImpl>();
  pEdit->EnableRefresh(false);
  pEdit->SetFontMap(&font_map);
  pEdit->SetPlateRect(CFX_FloatRect(rcClient.left, 0.0f, rcClient.right, 0.0f));

  float fFontSize = widget_->GetFontSize();
  pEdit->SetFontSize(FXSYS_IsFloatZero(fFontSize) ? 12.0f : fFontSize);
  pEdit->Initialize();

  fxcrt::ostringstream sList;
  float fy = rcClient.top;

  int32_t nTop = pField->GetTopVisibleIndex();
  int32_t nCount = pField->CountOptions();
  int32_t nSelCount = pField->CountSelectedItems();

  for (int32_t i = nTop; i < nCount; ++i) {
    bool bSelected = false;
    for (int32_t j = 0; j < nSelCount; ++j) {
      if (pField->GetSelectedIndex(j) == i) {
        bSelected = true;
        break;
      }
    }

    pEdit->SetText(pField->GetOptionLabel(i));
    pEdit->Paint();

    CFX_FloatRect rcContent = pEdit->GetContentRect();
    float fItemHeight = rcContent.Height();

    if (bSelected) {
      CFX_FloatRect rcItem =
          CFX_FloatRect(rcClient.left, fy - fItemHeight, rcClient.right, fy);
      {
        AutoClosedQCommand q(&sList);
        sList << GetColorAppStream(CFX_Color(CFX_Color::Type::kRGB, 0,
                                             51.0f / 255.0f, 113.0f / 255.0f),
                                   true)
              << rcItem.left << " " << rcItem.bottom << " " << rcItem.Width()
              << " " << rcItem.Height() << " " << kAppendRectOperator << " "
              << kFillOperator << "\n";
      }

      AutoClosedCommand bt(&sList, kTextBeginOperator, kTextEndOperator);
      sList << GetColorAppStream(CFX_Color(CFX_Color::Type::kGray, 1), true)
            << GetEditAppStream(pEdit.get(), CFX_PointF(0.0f, fy), true, 0);
    } else {
      CFX_Color crText = widget_->GetTextPWLColor();

      AutoClosedCommand bt(&sList, kTextBeginOperator, kTextEndOperator);
      sList << GetColorAppStream(crText, true)
            << GetEditAppStream(pEdit.get(), CFX_PointF(0.0f, fy), true, 0);
    }

    fy -= fItemHeight;
  }

  if (sList.tellp() > 0) {
    sBody << "/Tx ";
    AutoClosedCommand bmc(&sBody, kMarkedSequenceBeginOperator,
                          kMarkedSequenceEndOperator);
    AutoClosedQCommand q(&sBody);

    sBody << rcClient.left << " " << rcClient.bottom << " " << rcClient.Width()
          << " " << rcClient.Height() << " " << kAppendRectOperator << "\n"
          << kSetNonZeroWindingClipOperator << "\n"
          << kEndPathNoFillOrStrokeOperator << "\n"
          << sList.str();
  }
  Write("N",
        GetBackgroundAppStream() + GetBorderAppStream() + ByteString(sBody),
        ByteString());
}

void CPDFSDK_AppStream::SetAsTextField(absl::optional<WideString> sValue) {
  CPDF_FormControl* pControl = widget_->GetFormControl();
  CPDF_FormField* pField = pControl->GetField();
  fxcrt::ostringstream sBody;
  fxcrt::ostringstream sLines;

  // Font map must outlive |pEdit|.
  CPDF_BAFontMap font_map(widget_->GetPDFPage()->GetDocument(),
                          widget_->GetPDFAnnot()->GetAnnotDict(), "N");

  auto pEdit = std::make_unique<CPWL_EditImpl>();
  pEdit->EnableRefresh(false);
  pEdit->SetFontMap(&font_map);

  CFX_FloatRect rcClient = widget_->GetClientRect();
  pEdit->SetPlateRect(rcClient);
  pEdit->SetAlignmentH(pControl->GetControlAlignment());

  uint32_t dwFieldFlags = pField->GetFieldFlags();
  bool bMultiLine = dwFieldFlags & pdfium::form_flags::kTextMultiline;
  if (bMultiLine) {
    pEdit->SetMultiLine(true);
    pEdit->SetAutoReturn(true);
  } else {
    pEdit->SetAlignmentV(1);
  }

  uint16_t subWord = 0;
  if (dwFieldFlags & pdfium::form_flags::kTextPassword) {
    subWord = '*';
    pEdit->SetPasswordChar(subWord);
  }

  int nMaxLen = pField->GetMaxLen();
  bool bCharArray = dwFieldFlags & pdfium::form_flags::kTextComb;
  float fFontSize = widget_->GetFontSize();

#ifdef PDF_ENABLE_XFA
  if (!sValue.has_value() && widget_->GetMixXFAWidget())
    sValue = widget_->GetValue();
#endif  // PDF_ENABLE_XFA

  if (nMaxLen > 0) {
    if (bCharArray) {
      pEdit->SetCharArray(nMaxLen);
      if (FXSYS_IsFloatZero(fFontSize)) {
        fFontSize = CPWL_Edit::GetCharArrayAutoFontSize(
            font_map.GetPDFFont(0).Get(), rcClient, nMaxLen);
      }
    } else {
      if (sValue.has_value())
        nMaxLen = pdfium::base::checked_cast<int>(sValue.value().GetLength());
      pEdit->SetLimitChar(nMaxLen);
    }
  }

  if (FXSYS_IsFloatZero(fFontSize))
    pEdit->SetAutoFontSize(true);
  else
    pEdit->SetFontSize(fFontSize);

  pEdit->Initialize();
  pEdit->SetText(sValue.value_or(pField->GetValue()));
  pEdit->Paint();

  CFX_FloatRect rcContent = pEdit->GetContentRect();
  ByteString sEdit =
      GetEditAppStream(pEdit.get(), CFX_PointF(), !bCharArray, subWord);

  if (sEdit.GetLength() > 0) {
    sBody << "/Tx ";
    AutoClosedCommand bmc(&sBody, kMarkedSequenceBeginOperator,
                          kMarkedSequenceEndOperator);
    AutoClosedQCommand q(&sBody);

    if (rcContent.Width() > rcClient.Width() ||
        rcContent.Height() > rcClient.Height()) {
      sBody << rcClient.left << " " << rcClient.bottom << " "
            << rcClient.Width() << " " << rcClient.Height() << " "
            << kAppendRectOperator << "\n"
            << kSetNonZeroWindingClipOperator << "\n"
            << kEndPathNoFillOrStrokeOperator << "\n";
    }
    CFX_Color crText = widget_->GetTextPWLColor();

    AutoClosedCommand bt(&sBody, kTextBeginOperator, kTextEndOperator);
    sBody << GetColorAppStream(crText, true) << sEdit;
  }

  if (bCharArray) {
    switch (widget_->GetBorderStyle()) {
      case BorderStyle::kSolid: {
        ByteString sColor =
            GetColorAppStream(widget_->GetBorderPWLColor(), false);
        if (sColor.GetLength() > 0) {
          AutoClosedQCommand q(&sLines);
          sLines << widget_->GetBorderWidth() << " " << kSetLineWidthOperator
                 << "\n"
                 << GetColorAppStream(widget_->GetBorderPWLColor(), false)
                 << " 2 " << kSetLineCapStyleOperator << " 0 "
                 << kSetLineJoinStyleOperator << "\n";

          for (int32_t i = 1; i < nMaxLen; ++i) {
            sLines << rcClient.left +
                          ((rcClient.right - rcClient.left) / nMaxLen) * i
                   << " " << rcClient.bottom << " " << kMoveToOperator << "\n"
                   << rcClient.left +
                          ((rcClient.right - rcClient.left) / nMaxLen) * i
                   << " " << rcClient.top << " " << kLineToOperator << " "
                   << kStrokeOperator << "\n";
          }
        }
        break;
      }
      case BorderStyle::kDash: {
        ByteString sColor =
            GetColorAppStream(widget_->GetBorderPWLColor(), false);
        if (sColor.GetLength() > 0) {
          CPWL_Dash dsBorder = CPWL_Dash(3, 3, 0);
          AutoClosedQCommand q(&sLines);
          sLines << widget_->GetBorderWidth() << " " << kSetLineWidthOperator
                 << "\n"
                 << GetColorAppStream(widget_->GetBorderPWLColor(), false)
                 << "[" << dsBorder.nDash << " " << dsBorder.nGap << "] "
                 << dsBorder.nPhase << " " << kSetDashOperator << "\n";

          for (int32_t i = 1; i < nMaxLen; ++i) {
            sLines << rcClient.left +
                          ((rcClient.right - rcClient.left) / nMaxLen) * i
                   << " " << rcClient.bottom << " " << kMoveToOperator << "\n"
                   << rcClient.left +
                          ((rcClient.right - rcClient.left) / nMaxLen) * i
                   << " " << rcClient.top << " " << kLineToOperator << " "
                   << kStrokeOperator << "\n";
          }
        }
        break;
      }
      default:
        break;
    }
  }

  Write("N",
        GetBackgroundAppStream() + GetBorderAppStream() + ByteString(sLines) +
            ByteString(sBody),
        ByteString());
}

void CPDFSDK_AppStream::AddImage(const ByteString& sAPType,
                                 CPDF_Stream* pImage) {
  CPDF_Stream* pStream = dict_->GetStreamFor(sAPType);
  CPDF_Dictionary* pStreamDict = pStream->GetDict();
  ByteString sImageAlias = "IMG";

  CPDF_Dictionary* pImageDict = pImage->GetDict();
  if (pImageDict)
    sImageAlias = pImageDict->GetStringFor("Name");

  CPDF_Dictionary* pStreamResList = GetOrCreateDict(pStreamDict, "Resources");
  CPDF_Dictionary* pXObject =
      pStreamResList->SetNewFor<CPDF_Dictionary>("XObject");
  pXObject->SetNewFor<CPDF_Reference>(sImageAlias,
                                      widget_->GetPageView()->GetPDFDocument(),
                                      pImage->GetObjNum());
}

void CPDFSDK_AppStream::Write(const ByteString& sAPType,
                              const ByteString& sContents,
                              const ByteString& sAPState) {
  CPDF_Dictionary* pParentDict;
  ByteString key;
  if (sAPState.IsEmpty()) {
    pParentDict = dict_.Get();
    key = sAPType;
  } else {
    pParentDict = GetOrCreateDict(dict_.Get(), sAPType);
    key = sAPState;
  }

  // If `pStream` is created by CreateModifiedAPStream(), then it is safe to
  // edit, as it is not shared.
  CPDF_Stream* pStream = pParentDict->GetStreamFor(key);
  CPDF_Document* doc = widget_->GetPageView()->GetPDFDocument();
  if (!doc->IsModifiedAPStream(pStream)) {
    pStream = doc->CreateModifiedAPStream();
    pParentDict->SetNewFor<CPDF_Reference>(key, doc, pStream->GetObjNum());
  }

  CPDF_Dictionary* pStreamDict = pStream->GetDict();
  if (!pStreamDict) {
    auto pNewDict =
        widget_->GetPDFAnnot()->GetDocument()->New<CPDF_Dictionary>();
    pStreamDict = pNewDict.Get();
    pStreamDict->SetNewFor<CPDF_Name>("Type", "XObject");
    pStreamDict->SetNewFor<CPDF_Name>("Subtype", "Form");
    pStreamDict->SetNewFor<CPDF_Number>("FormType", 1);
    pStream->InitStream({}, std::move(pNewDict));
  }
  pStreamDict->SetMatrixFor("Matrix", widget_->GetMatrix());
  pStreamDict->SetRectFor("BBox", widget_->GetRotatedRect());
  pStream->SetDataAndRemoveFilter(sContents.raw_span());
}

void CPDFSDK_AppStream::Remove(const ByteString& sAPType) {
  dict_->RemoveFor(sAPType);
}

ByteString CPDFSDK_AppStream::GetBackgroundAppStream() const {
  CFX_Color crBackground = widget_->GetFillPWLColor();
  if (crBackground.nColorType != CFX_Color::Type::kTransparent)
    return GetRectFillAppStream(widget_->GetRotatedRect(), crBackground);

  return ByteString();
}

ByteString CPDFSDK_AppStream::GetBorderAppStream() const {
  CFX_FloatRect rcWindow = widget_->GetRotatedRect();
  CFX_Color crBorder = widget_->GetBorderPWLColor();
  CFX_Color crBackground = widget_->GetFillPWLColor();
  CFX_Color crLeftTop;
  CFX_Color crRightBottom;

  float fBorderWidth = static_cast<float>(widget_->GetBorderWidth());
  CPWL_Dash dsBorder(3, 0, 0);

  BorderStyle nBorderStyle = widget_->GetBorderStyle();
  switch (nBorderStyle) {
    case BorderStyle::kDash:
      dsBorder = CPWL_Dash(3, 3, 0);
      break;
    case BorderStyle::kBeveled:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 1);
      crRightBottom = crBackground / 2.0f;
      break;
    case BorderStyle::kInset:
      fBorderWidth *= 2;
      crLeftTop = CFX_Color(CFX_Color::Type::kGray, 0.5);
      crRightBottom = CFX_Color(CFX_Color::Type::kGray, 0.75);
      break;
    default:
      break;
  }

  return GetBorderAppStreamInternal(rcWindow, fBorderWidth, crBorder, crLeftTop,
                                    crRightBottom, nBorderStyle, dsBorder);
}
