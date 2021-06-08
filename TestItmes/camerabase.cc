#include "stdafx.h"

#include "camerabase.h"

namespace media {

    // This list is ordered by precedence of use.
    static VideoPixelFormat const kSupportedCapturePixelFormats[] = {
        PIXEL_FORMAT_I420,  PIXEL_FORMAT_YV12, PIXEL_FORMAT_NV12,
        PIXEL_FORMAT_NV21,  PIXEL_FORMAT_UYVY, PIXEL_FORMAT_YUY2,
        PIXEL_FORMAT_RGB24, PIXEL_FORMAT_ARGB, PIXEL_FORMAT_MJPEG,
    };

    VideoCaptureFormat::VideoCaptureFormat()
        : frame_rate(0.0f), pixel_format(PIXEL_FORMAT_UNKNOWN) {}

    VideoCaptureFormat::VideoCaptureFormat(const gfx::Size& frame_size,
        float frame_rate,
        VideoPixelFormat pixel_format)
        : frame_size(frame_size),
        frame_rate(frame_rate),
        pixel_format(pixel_format) {}

    bool VideoCaptureFormat::IsValid() const {
        return true;
    }

    // static
    bool VideoCaptureFormat::ComparePixelFormatPreference(
        const VideoPixelFormat& lhs,
        const VideoPixelFormat& rhs) {
        auto* format_lhs = std::find(
            kSupportedCapturePixelFormats,
            kSupportedCapturePixelFormats + 9,
            lhs);
        auto* format_rhs = std::find(
            kSupportedCapturePixelFormats,
            kSupportedCapturePixelFormats + 9,
            rhs);
        return format_lhs < format_rhs;
    }

    std::wstring WStringFromGUID(REFGUID rguid) {
        // This constant counts the number of characters in the formatted string,
        // including the null termination character.
        constexpr int kGuidStringCharacters =
            1 + 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1;
        wchar_t guid_string[kGuidStringCharacters];
        StringCchPrintfW(
            guid_string, kGuidStringCharacters,
            L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", rguid.Data1,
            rguid.Data2, rguid.Data3, rguid.Data4[0], rguid.Data4[1], rguid.Data4[2],
            rguid.Data4[3], rguid.Data4[4], rguid.Data4[5], rguid.Data4[6],
            rguid.Data4[7]);
        return std::wstring(guid_string, kGuidStringCharacters - 1);
    }


    VideoCaptureDeviceDescriptor::VideoCaptureDeviceDescriptor()
        : facing(VideoFacingMode::MEDIA_VIDEO_FACING_NONE),
        capture_api(VideoCaptureApi::UNKNOWN),
        transport_type(VideoCaptureTransportType::OTHER_TRANSPORT) {}

    VideoCaptureDeviceDescriptor::VideoCaptureDeviceDescriptor(
        const std::string& display_name,
        const std::string& device_id,
        VideoCaptureApi capture_api,
        const VideoCaptureControlSupport& control_support,
        VideoCaptureTransportType transport_type)
        : device_id(device_id),
        facing(VideoFacingMode::MEDIA_VIDEO_FACING_NONE),
        capture_api(capture_api),
        transport_type(transport_type),
        display_name_(display_name),
        control_support_(control_support) {}

    VideoCaptureDeviceDescriptor::VideoCaptureDeviceDescriptor(
        const std::string& display_name,
        const std::string& device_id,
        const std::string& model_id,
        VideoCaptureApi capture_api,
        const VideoCaptureControlSupport& control_support,
        VideoCaptureTransportType transport_type,
        VideoFacingMode facing)
        : device_id(device_id),
        model_id(model_id),
        facing(facing),
        capture_api(capture_api),
        transport_type(transport_type),
        display_name_(display_name),
        control_support_(control_support) {}

    VideoCaptureDeviceDescriptor::~VideoCaptureDeviceDescriptor() = default;

    VideoCaptureDeviceDescriptor::VideoCaptureDeviceDescriptor(
        const VideoCaptureDeviceDescriptor & other) = default;

    bool VideoCaptureDeviceDescriptor::operator<(
        const VideoCaptureDeviceDescriptor & other) const {
        static constexpr int kFacingMapping[NUM_MEDIA_VIDEO_FACING_MODES] = { 0, 2, 1 };
        static_assert(kFacingMapping[MEDIA_VIDEO_FACING_NONE] == 0,
            "FACING_NONE has a wrong value");
        static_assert(kFacingMapping[MEDIA_VIDEO_FACING_ENVIRONMENT] == 1,
            "FACING_ENVIRONMENT has a wrong value");
        static_assert(kFacingMapping[MEDIA_VIDEO_FACING_USER] == 2,
            "FACING_USER has a wrong value");
        if (kFacingMapping[facing] != kFacingMapping[other.facing])
            return kFacingMapping[facing] > kFacingMapping[other.facing];
        if (device_id != other.device_id)
            return device_id < other.device_id;
        return capture_api < other.capture_api;
    }


    // Compares the priority of the capture formats. Returns true if |lhs| is the
    // preferred capture format in comparison with |rhs|. Returns false otherwise.
    bool CompareCapability(const VideoCaptureFormat& requested,
        const VideoCaptureFormat& lhs,
        const VideoCaptureFormat& rhs) {
        // When 16-bit format or NV12 is requested and available, avoid other formats.
        // If both lhs and rhs are 16-bit, we still need to compare them based on
        // height, width and frame rate.
        const bool use_requested =
            (requested.pixel_format == media::PIXEL_FORMAT_Y16) ||
            (requested.pixel_format == media::PIXEL_FORMAT_NV12);
        if (use_requested && lhs.pixel_format != rhs.pixel_format) {
            if (lhs.pixel_format == requested.pixel_format)
                return true;
            if (rhs.pixel_format == requested.pixel_format)
                return false;
        }
        const int diff_height_lhs =
            std::abs(lhs.frame_size.height - requested.frame_size.height);
        const int diff_height_rhs =
            std::abs(rhs.frame_size.height - requested.frame_size.height);
        if (diff_height_lhs != diff_height_rhs)
            return diff_height_lhs < diff_height_rhs;

        const int diff_width_lhs =
            std::abs(lhs.frame_size.width - requested.frame_size.width);
        const int diff_width_rhs =
            std::abs(rhs.frame_size.width - requested.frame_size.width);
        if (diff_width_lhs != diff_width_rhs)
            return diff_width_lhs < diff_width_rhs;

        const float diff_fps_lhs = std::fabs(lhs.frame_rate - requested.frame_rate);
        const float diff_fps_rhs = std::fabs(rhs.frame_rate - requested.frame_rate);
        if (diff_fps_lhs != diff_fps_rhs)
            return diff_fps_lhs < diff_fps_rhs;

        return VideoCaptureFormat::ComparePixelFormatPreference(lhs.pixel_format,
            rhs.pixel_format);
    }

    const CapabilityWin& GetBestMatchedCapability(
        const VideoCaptureFormat& requested,
        const CapabilityList& capabilities) {
        DCHECK(!capabilities.empty());
        const CapabilityWin* best_match = &(*capabilities.begin());
        for (const CapabilityWin& capability : capabilities) {
            if (CompareCapability(requested, capability.supported_format,
                best_match->supported_format)) {
                best_match = &capability;
            }
        }
        return *best_match;
    }


    SinkFilterObserver::~SinkFilterObserver() {
    }


    SinkFilter::SinkFilter(SinkFilterObserver* observer) {
        input_pin_ = new SinkInputPin(this, observer);
    }

    void SinkFilter::SetRequestedMediaFormat(VideoPixelFormat pixel_format,
        float frame_rate,
        const BITMAPINFOHEADER& info_header) {
        input_pin_->SetRequestedMediaFormat(pixel_format, frame_rate, info_header);
    }

    size_t SinkFilter::NoOfPins() {
        return 1;
    }

    IPin* SinkFilter::GetPin(int index) {
        return index == 0 ? input_pin_.Get() : nullptr;
    }

    HRESULT SinkFilter::GetClassID(CLSID* clsid) {
        *clsid = __uuidof(SinkFilter);
        return S_OK;
    }

    SinkFilter::~SinkFilter() {
        input_pin_->SetOwner(nullptr);
    }
}  // namespace


