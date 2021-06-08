#pragma once
#include <dshow.h>
#include <stdint.h>
#include <vidcap.h>
#include <wrl/client.h>

#include <map>
#include <string>
#include <memory>

#include <string>
#include <vector>
#include <wingdi.h>
#include <list>
#include <assert.h>
#include <iostream>
#include <strsafe.h>
#include <strmif.h>

typedef LONGLONG REFERENCE_TIME;

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() false
#else
#define DCHECK_IS_ON() true
#endif

#if DCHECK_IS_ON()

#define DCHECK(condition)                                                     \
  assert(condition)

#define DPCHECK(condition)                                                     \
  assert(condition)

#else

#define DCHECK(condition) assert(condition)
#define DPCHECK(condition) assert(condition)

#endif

#define DLOG(severity)              std::cout << severity                             
#define DVLOG(severity)              std::cout << severity                             
#define NOTREACHED() DCHECK(false)

#define DLOG_IF(severity, condition) LOG_IF(severity, condition)


#if DCHECK_IS_ON()
#define DLOG_IF_FAILED_WITH_HRESULT(message, hr)                       \
  {                                                                    \
    if(FAILED(hr))                                                     \
    {                                                                  \
        std::cout << (message) << ": " << hr;                          \
    }                                                                  \
  }                                                                    
#else
#define DLOG_IF_FAILED_WITH_HRESULT(message, hr) \
  {}
#endif

namespace gfx
{
    struct Size
    {
        int width;
        int height;
        bool operator==(const Size& other) const {
            return width == other.width && height == other.height;
        }
        void SetSize(int Width, int Height)
        {
            width = Width;
            height = Height;
        }
    };
}

namespace media {
	struct VideoCaptureControlSupport {
		bool pan = false;
		bool tilt = false;
		bool zoom = false;
	};

    enum VideoPixelFormat {
        PIXEL_FORMAT_UNKNOWN = 0,  // Unknown or unspecified format value.
        PIXEL_FORMAT_I420 =
        1,  // 12bpp YUV planar 1x1 Y, 2x2 UV samples, a.k.a. YU12.

    // Note: Chrome does not actually support YVU compositing, so you probably
    // don't actually want to use this. See http://crbug.com/784627.
        PIXEL_FORMAT_YV12 = 2,  // 12bpp YVU planar 1x1 Y, 2x2 VU samples.

        PIXEL_FORMAT_I422 = 3,   // 16bpp YUV planar 1x1 Y, 2x1 UV samples.
        PIXEL_FORMAT_I420A = 4,  // 20bpp YUVA planar 1x1 Y, 2x2 UV, 1x1 A samples.
        PIXEL_FORMAT_I444 = 5,   // 24bpp YUV planar, no subsampling.
        PIXEL_FORMAT_NV12 =
        6,  // 12bpp with Y plane followed by a 2x2 interleaved UV plane.
        PIXEL_FORMAT_NV21 =
        7,  // 12bpp with Y plane followed by a 2x2 interleaved VU plane.
        PIXEL_FORMAT_UYVY =
        8,  // 16bpp interleaved 2x1 U, 1x1 Y, 2x1 V, 1x1 Y samples.
        PIXEL_FORMAT_YUY2 =
        9,  // 16bpp interleaved 1x1 Y, 2x1 U, 1x1 Y, 2x1 V samples.
        PIXEL_FORMAT_ARGB = 10,   // 32bpp BGRA (byte-order), 1 plane.
        PIXEL_FORMAT_XRGB = 11,   // 24bpp BGRX (byte-order), 1 plane.
        PIXEL_FORMAT_RGB24 = 12,  // 24bpp BGR (byte-order), 1 plane.

        /* PIXEL_FORMAT_RGB32 = 13,  Deprecated */
        PIXEL_FORMAT_MJPEG = 14,  // MJPEG compressed.
        /* PIXEL_FORMAT_MT21 = 15,  Deprecated */

        // The P* in the formats below designates the number of bits per pixel
        // component. I.e. P9 is 9-bits per pixel component, P10 is 10-bits per pixel
        // component, etc.
        PIXEL_FORMAT_YUV420P9 = 16,
        PIXEL_FORMAT_YUV420P10 = 17,
        PIXEL_FORMAT_YUV422P9 = 18,
        PIXEL_FORMAT_YUV422P10 = 19,
        PIXEL_FORMAT_YUV444P9 = 20,
        PIXEL_FORMAT_YUV444P10 = 21,
        PIXEL_FORMAT_YUV420P12 = 22,
        PIXEL_FORMAT_YUV422P12 = 23,
        PIXEL_FORMAT_YUV444P12 = 24,

        /* PIXEL_FORMAT_Y8 = 25, Deprecated */
        PIXEL_FORMAT_Y16 = 26,  // single 16bpp plane.

        PIXEL_FORMAT_ABGR = 27,  // 32bpp RGBA (byte-order), 1 plane.
        PIXEL_FORMAT_XBGR = 28,  // 24bpp RGBX (byte-order), 1 plane.

        PIXEL_FORMAT_P016LE = 29,  // 24bpp NV12, 16 bits per channel

        PIXEL_FORMAT_XR30 =
        30,  // 32bpp BGRX, 10 bits per channel, 2 bits ignored, 1 plane
        PIXEL_FORMAT_XB30 =
        31,  // 32bpp RGBX, 10 bits per channel, 2 bits ignored, 1 plane

        PIXEL_FORMAT_BGRA = 32,  // 32bpp ARGB (byte-order), 1 plane.

        PIXEL_FORMAT_RGBAF16 = 33,  // Half float RGBA, 1 plane.

        // Please update UMA histogram enumeration when adding new formats here.
        PIXEL_FORMAT_MAX =
        PIXEL_FORMAT_RGBAF16,  // Must always be equal to largest entry logged.
    };


    // Policies for capture devices that have source content that varies in size.
    // It is up to the implementation how the captured content will be transformed
    // (e.g., scaling and/or letterboxing) in order to produce video frames that
    // strictly adheree to one of these policies.
    enum class ResolutionChangePolicy {
        // Capture device outputs a fixed resolution all the time. The resolution of
        // the first frame is the resolution for all frames.
        FIXED_RESOLUTION,

        // Capture device is allowed to output frames of varying resolutions. The
        // width and height will not exceed the maximum dimensions specified. The
        // aspect ratio of the frames will match the aspect ratio of the maximum
        // dimensions as closely as possible.
        FIXED_ASPECT_RATIO,

        // Capture device is allowed to output frames of varying resolutions not
        // exceeding the maximum dimensions specified.
        ANY_WITHIN_LIMIT,

        // Must always be equal to largest entry in the enum.
        LAST = ANY_WITHIN_LIMIT,
    };

    // Potential values of the googPowerLineFrequency optional constraint passed to
    // getUserMedia. Note that the numeric values are currently significant, and are
    // used to map enum values to corresponding frequency values.
    // TODO(ajose): http://crbug.com/525167 Consider making this a class.
    enum class PowerLineFrequency {
        FREQUENCY_DEFAULT = 0,
        FREQUENCY_50HZ = 50,
        FREQUENCY_60HZ = 60,
        FREQUENCY_MAX = FREQUENCY_60HZ
    };

    enum class VideoCaptureBufferType {
        kSharedMemory,
        kSharedMemoryViaRawFileDescriptor,
        kMailboxHolder,
        kGpuMemoryBuffer
    };

    // WARNING: Do not change the values assigned to the entries. They are used for
    // UMA logging.
    enum class VideoCaptureError {
        kNone = 0,
        kVideoCaptureControllerInvalidOrUnsupportedVideoCaptureParametersRequested =
        1,
        kVideoCaptureControllerIsAlreadyInErrorState = 2,
        kVideoCaptureManagerDeviceConnectionLost = 3,
        kFrameSinkVideoCaptureDeviceAleradyEndedOnFatalError = 4,
        kFrameSinkVideoCaptureDeviceEncounteredFatalError = 5,
        kV4L2FailedToOpenV4L2DeviceDriverFile = 6,
        kV4L2ThisIsNotAV4L2VideoCaptureDevice = 7,
        kV4L2FailedToFindASupportedCameraFormat = 8,
        kV4L2FailedToSetVideoCaptureFormat = 9,
        kV4L2UnsupportedPixelFormat = 10,
        kV4L2FailedToSetCameraFramerate = 11,
        kV4L2ErrorRequestingMmapBuffers = 12,
        kV4L2AllocateBufferFailed = 13,
        kV4L2VidiocStreamonFailed = 14,
        kV4L2VidiocStreamoffFailed = 15,
        kV4L2FailedToVidiocReqbufsWithCount0 = 16,
        kV4L2PollFailed = 17,
        kV4L2MultipleContinuousTimeoutsWhileReadPolling = 18,
        kV4L2FailedToDequeueCaptureBuffer = 19,
        kV4L2FailedToEnqueueCaptureBuffer = 20,
        kSingleClientVideoCaptureHostLostConnectionToDevice = 21,
        kSingleClientVideoCaptureDeviceLaunchAborted = 22,
        kDesktopCaptureDeviceWebrtcDesktopCapturerHasFailed = 23,
        kFileVideoCaptureDeviceCouldNotOpenVideoFile = 24,
        kDeviceCaptureLinuxFailedToCreateVideoCaptureDelegate = 25,
        kErrorFakeDeviceIntentionallyEmittingErrorEvent = 26,
        kDeviceClientTooManyFramesDroppedY16 = 28,
        kDeviceMediaToMojoAdapterEncounteredUnsupportedBufferType = 29,
        kVideoCaptureManagerProcessDeviceStartQueueDeviceInfoNotFound = 30,
        kInProcessDeviceLauncherFailedToCreateDeviceInstance = 31,
        kServiceDeviceLauncherLostConnectionToDeviceFactoryDuringDeviceStart = 32,
        kServiceDeviceLauncherServiceRespondedWithDeviceNotFound = 33,
        kServiceDeviceLauncherConnectionLostWhileWaitingForCallback = 34,
        kIntentionalErrorRaisedByUnitTest = 35,
        kCrosHalV3FailedToStartDeviceThread = 36,
        kCrosHalV3DeviceDelegateMojoConnectionError = 37,
        kCrosHalV3DeviceDelegateFailedToGetCameraInfo = 38,
        kCrosHalV3DeviceDelegateMissingSensorOrientationInfo = 39,
        kCrosHalV3DeviceDelegateFailedToOpenCameraDevice = 40,
        kCrosHalV3DeviceDelegateFailedToInitializeCameraDevice = 41,
        kCrosHalV3DeviceDelegateFailedToConfigureStreams = 42,
        kCrosHalV3DeviceDelegateWrongNumberOfStreamsConfigured = 43,
        kCrosHalV3DeviceDelegateFailedToGetDefaultRequestSettings = 44,
        kCrosHalV3BufferManagerHalRequestedTooManyBuffers = 45,
        kCrosHalV3BufferManagerFailedToCreateGpuMemoryBuffer = 46,
        kCrosHalV3BufferManagerFailedToMapGpuMemoryBuffer = 47,
        kCrosHalV3BufferManagerUnsupportedVideoPixelFormat = 48,
        kCrosHalV3BufferManagerFailedToDupFd = 49,
        kCrosHalV3BufferManagerFailedToWrapGpuMemoryHandle = 50,
        kCrosHalV3BufferManagerFailedToRegisterBuffer = 51,
        kCrosHalV3BufferManagerProcessCaptureRequestFailed = 52,
        kCrosHalV3BufferManagerInvalidPendingResultId = 53,
        kCrosHalV3BufferManagerReceivedDuplicatedPartialMetadata = 54,
        kCrosHalV3BufferManagerIncorrectNumberOfOutputBuffersReceived = 55,
        kCrosHalV3BufferManagerInvalidTypeOfOutputBuffersReceived = 56,
        kCrosHalV3BufferManagerReceivedMultipleResultBuffersForFrame = 57,
        kCrosHalV3BufferManagerUnknownStreamInCamera3NotifyMsg = 58,
        kCrosHalV3BufferManagerReceivedInvalidShutterTime = 59,
        kCrosHalV3BufferManagerFatalDeviceError = 60,
        kCrosHalV3BufferManagerReceivedFrameIsOutOfOrder = 61,
        kCrosHalV3BufferManagerFailedToUnwrapReleaseFenceFd = 62,
        kCrosHalV3BufferManagerSyncWaitOnReleaseFenceTimedOut = 63,
        kCrosHalV3BufferManagerInvalidJpegBlob = 64,
        kAndroidFailedToAllocate = 65,
        kAndroidFailedToStartCapture = 66,
        kAndroidFailedToStopCapture = 67,
        kAndroidApi1CameraErrorCallbackReceived = 68,
        kAndroidApi2CameraDeviceErrorReceived = 69,
        kAndroidApi2CaptureSessionConfigureFailed = 70,
        kAndroidApi2ImageReaderUnexpectedImageFormat = 71,
        kAndroidApi2ImageReaderSizeDidNotMatchImageSize = 72,
        kAndroidApi2ErrorRestartingPreview = 73,
        kAndroidScreenCaptureUnsupportedFormat = 74,
        kAndroidScreenCaptureFailedToStartCaptureMachine = 75,
        kAndroidScreenCaptureTheUserDeniedScreenCapture = 76,
        kAndroidScreenCaptureFailedToStartScreenCapture = 77,
        kWinDirectShowCantGetCaptureFormatSettings = 78,
        kWinDirectShowFailedToGetNumberOfCapabilities = 79,
        kWinDirectShowFailedToGetCaptureDeviceCapabilities = 80,
        kWinDirectShowFailedToSetCaptureDeviceOutputFormat = 81,
        kWinDirectShowFailedToConnectTheCaptureGraph = 82,
        kWinDirectShowFailedToPauseTheCaptureDevice = 83,
        kWinDirectShowFailedToStartTheCaptureDevice = 84,
        kWinDirectShowFailedToStopTheCaptureGraph = 85,
        kWinMediaFoundationEngineIsNull = 86,
        kWinMediaFoundationEngineGetSourceFailed = 87,
        kWinMediaFoundationFillPhotoCapabilitiesFailed = 88,
        kWinMediaFoundationFillVideoCapabilitiesFailed = 89,
        kWinMediaFoundationNoVideoCapabilityFound = 90,
        kWinMediaFoundationGetAvailableDeviceMediaTypeFailed = 91,
        kWinMediaFoundationSetCurrentDeviceMediaTypeFailed = 92,
        kWinMediaFoundationEngineGetSinkFailed = 93,
        kWinMediaFoundationSinkQueryCapturePreviewInterfaceFailed = 94,
        kWinMediaFoundationSinkRemoveAllStreamsFailed = 95,
        kWinMediaFoundationCreateSinkVideoMediaTypeFailed = 96,
        kWinMediaFoundationConvertToVideoSinkMediaTypeFailed = 97,
        kWinMediaFoundationSinkAddStreamFailed = 98,
        kWinMediaFoundationSinkSetSampleCallbackFailed = 99,
        kWinMediaFoundationEngineStartPreviewFailed = 100,
        kWinMediaFoundationGetMediaEventStatusFailed = 101,
        kMacSetCaptureDeviceFailed = 102,
        kMacCouldNotStartCaptureDevice = 103,
        kMacReceivedFrameWithUnexpectedResolution = 104,
        kMacUpdateCaptureResolutionFailed = 105,
        kMacDeckLinkDeviceIdNotFoundInTheSystem = 106,
        kMacDeckLinkErrorQueryingInputInterface = 107,
        kMacDeckLinkErrorCreatingDisplayModeIterator = 108,
        kMacDeckLinkCouldNotFindADisplayMode = 109,
        kMacDeckLinkCouldNotSelectTheVideoFormatWeLike = 110,
        kMacDeckLinkCouldNotStartCapturing = 111,
        kMacDeckLinkUnsupportedPixelFormat = 112,
        kMacAvFoundationReceivedAVCaptureSessionRuntimeErrorNotification = 113,
        kAndroidApi2ErrorConfiguringCamera = 114,
        kCrosHalV3DeviceDelegateFailedToFlush = 115,
        kFuchsiaCameraDeviceDisconnected = 116,
        kFuchsiaCameraStreamDisconnected = 117,
        kFuchsiaSysmemDidNotSetImageFormat = 118,
        kFuchsiaSysmemInvalidBufferIndex = 119,
        kFuchsiaSysmemInvalidBufferSize = 120,
        kFuchsiaUnsupportedPixelFormat = 121,
        kFuchsiaFailedToMapSysmemBuffer = 122,
        kCrosHalV3DeviceContextDuplicatedClient = 123,
        kDesktopCaptureDeviceMacFailedStreamCreate = 124,
        kDesktopCaptureDeviceMacFailedStreamStart = 125,
        kCrosHalV3BufferManagerFailedToReserveBuffers = 126,
        kMaxValue = 126
    };

    // WARNING: Do not change the values assigned to the entries. They are used for
    // UMA logging.
    enum class VideoCaptureFrameDropReason {
        kNone = 0,
        kDeviceClientFrameHasInvalidFormat = 1,
        kDeviceClientLibyuvConvertToI420Failed = 3,
        kV4L2BufferErrorFlagWasSet = 4,
        kV4L2InvalidNumberOfBytesInBuffer = 5,
        kAndroidThrottling = 6,
        kAndroidGetByteArrayElementsFailed = 7,
        kAndroidApi1UnexpectedDataLength = 8,
        kAndroidApi2AcquiredImageIsNull = 9,
        kWinDirectShowUnexpectedSampleLength = 10,
        kWinDirectShowFailedToGetMemoryPointerFromMediaSample = 11,
        kWinMediaFoundationReceivedSampleIsNull = 12,
        kWinMediaFoundationLockingBufferDelieveredNullptr = 13,
        kWinMediaFoundationGetBufferByIndexReturnedNull = 14,
        kBufferPoolMaxBufferCountExceeded = 15,
        kBufferPoolBufferAllocationFailed = 16,
        kVideoCaptureImplNotInStartedState = 17,
        kVideoCaptureImplFailedToWrapDataAsMediaVideoFrame = 18,
        kVideoTrackAdapterHasNoResolutionAdapters = 19,
        kResolutionAdapterFrameIsNotValid = 20,
        kResolutionAdapterWrappingFrameForCroppingFailed = 21,
        kResolutionAdapterTimestampTooCloseToPrevious = 22,
        kResolutionAdapterFrameRateIsHigherThanRequested = 23,
        kResolutionAdapterHasNoCallbacks = 24,
        kVideoTrackFrameDelivererNotEnabledReplacingWithBlackFrame = 25,
        kRendererSinkFrameDelivererIsNotStarted = 26,
        kMaxValue = 26
    };

    // Some drivers use rational time per frame instead of float frame rate, this
// constant k is used to convert between both: A fps -> [k/k*A] seconds/frame.
    const int kFrameRatePrecision = 10000;

    // Video capture format specification.
    // This class is used by the video capture device to specify the format of every
    // frame captured and returned to a client. It is also used to specify a
    // supported capture format by a device.
    struct VideoCaptureFormat {
        VideoCaptureFormat();
        VideoCaptureFormat(const gfx::Size& frame_size,
            float frame_rate,
            VideoPixelFormat pixel_format);

        static std::string ToString(const VideoCaptureFormat& format);

        // Compares the priority of the pixel formats. Returns true if |lhs| is the
        // preferred pixel format in comparison with |rhs|. Returns false otherwise.
        static bool ComparePixelFormatPreference(const VideoPixelFormat& lhs,
            const VideoPixelFormat& rhs);

        // Checks that all values are in the expected range. All limits are specified
        // in media::Limits.
        bool IsValid() const;

        bool operator==(const VideoCaptureFormat& other) const {
            return frame_size == other.frame_size && frame_rate == other.frame_rate &&
                pixel_format == other.pixel_format;
        }

        gfx::Size frame_size;
        float frame_rate;
        VideoPixelFormat pixel_format;
    };

    typedef std::vector<VideoCaptureFormat> VideoCaptureFormats;

    // Parameters for starting video capture.
    // This class is used by the client of a video capture device to specify the
    // format of frames in which the client would like to have captured frames
    // returned.
    struct VideoCaptureParams {
        // Result struct for SuggestContraints() method.
        struct SuggestedConstraints {
            gfx::Size min_frame_size;
            gfx::Size max_frame_size;
            bool fixed_aspect_ratio;
        };

        VideoCaptureParams();

        // Returns true if requested_format.IsValid() and all other values are within
        // their expected ranges.
        bool IsValid() const;

        // Computes and returns suggested capture constraints based on the requested
        // format and resolution change policy: minimum resolution, maximum
        // resolution, and whether a fixed aspect ratio is required.
        SuggestedConstraints SuggestConstraints() const;

        bool operator==(const VideoCaptureParams& other) const {
            return requested_format == other.requested_format &&
                resolution_change_policy == other.resolution_change_policy &&
                power_line_frequency == other.power_line_frequency;
        }

        // Requests a resolution and format at which the capture will occur.
        VideoCaptureFormat requested_format;

        VideoCaptureBufferType buffer_type;

        // Policy for resolution change.
        ResolutionChangePolicy resolution_change_policy;

        // User-specified power line frequency.
        PowerLineFrequency power_line_frequency;

        // Flag indicating if face detection should be enabled. This is for
        // allowing the driver to apply appropriate settings for optimal
        // exposures around the face area. Currently only applicable on
        // Android platform with Camera2 driver support.
        bool enable_face_detection;
    };

    struct CapabilityWin {
        CapabilityWin(int media_type_index, const VideoCaptureFormat& format)
            : media_type_index(media_type_index),
            supported_format(format),
            info_header(),
            stream_index(0) {}

        // Used by VideoCaptureDeviceWin.
        CapabilityWin(int media_type_index,
            const VideoCaptureFormat& format,
            const BITMAPINFOHEADER& info_header)
            : media_type_index(media_type_index),
            supported_format(format),
            info_header(info_header),
            stream_index(0) {}

        // Used by VideoCaptureDeviceMFWin.
        CapabilityWin(int media_type_index,
            const VideoCaptureFormat& format,
            int stream_index)
            : media_type_index(media_type_index),
            supported_format(format),
            info_header(),
            stream_index(stream_index) {}

        const int media_type_index;
        const VideoCaptureFormat supported_format;

        // |info_header| is only valid if DirectShow is used.
        const BITMAPINFOHEADER info_header;

        // |stream_index| is only valid if MediaFoundation is used.
        const int stream_index;
    };

    typedef std::list<CapabilityWin> CapabilityList;


    // A Java counterpart will be generated for this enum.
    // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.media
    enum class VideoCaptureApi {
        LINUX_V4L2_SINGLE_PLANE,
        WIN_MEDIA_FOUNDATION,
        WIN_MEDIA_FOUNDATION_SENSOR,
        WIN_DIRECT_SHOW,
        MACOSX_AVFOUNDATION,
        MACOSX_DECKLINK,
        ANDROID_API1,
        ANDROID_API2_LEGACY,
        ANDROID_API2_FULL,
        ANDROID_API2_LIMITED,
        FUCHSIA_CAMERA3,
        VIRTUAL_DEVICE,
        UNKNOWN
    };

    enum class VideoCaptureTransportType {
        // For AVFoundation Api, identify devices that are built-in or USB.
        MACOSX_USB_OR_BUILT_IN,
        OTHER_TRANSPORT
    };

    // Facing mode for video capture.
    // A Java counterpart will be generated for this enum.
    // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.media
    enum VideoFacingMode {
        MEDIA_VIDEO_FACING_NONE = 0,
        MEDIA_VIDEO_FACING_USER,
        MEDIA_VIDEO_FACING_ENVIRONMENT,

        NUM_MEDIA_VIDEO_FACING_MODES
    };

    // Represents information about a capture device as returned by
// VideoCaptureDeviceFactory::GetDeviceDescriptors().
// |device_id| represents a unique id of a physical device. Since the same
// physical device may be accessible through different APIs |capture_api|
// disambiguates the API.
// TODO(tommi): Given that this struct has become more complex with private
// members, methods that are not just direct getters/setters
// (e.g., GetNameAndModel), let's turn it into a class in order to properly
// conform with the style guide and protect the integrity of the data that the
// class owns.
    struct VideoCaptureDeviceDescriptor {
    public:
        VideoCaptureDeviceDescriptor();
        VideoCaptureDeviceDescriptor(
            const std::string& display_name,
            const std::string& device_id,
            VideoCaptureApi capture_api = VideoCaptureApi::UNKNOWN,
            const VideoCaptureControlSupport& control_support =
            VideoCaptureControlSupport(),
            VideoCaptureTransportType transport_type =
            VideoCaptureTransportType::OTHER_TRANSPORT);
        VideoCaptureDeviceDescriptor(
            const std::string& display_name,
            const std::string& device_id,
            const std::string& model_id,
            VideoCaptureApi capture_api,
            const VideoCaptureControlSupport& control_support,
            VideoCaptureTransportType transport_type =
            VideoCaptureTransportType::OTHER_TRANSPORT,
            VideoFacingMode facing = VideoFacingMode::MEDIA_VIDEO_FACING_NONE);
        VideoCaptureDeviceDescriptor(const VideoCaptureDeviceDescriptor& other);
        ~VideoCaptureDeviceDescriptor();

        // These operators are needed due to storing the name in an STL container.
        // In the shared build, all methods from the STL container will be exported
        // so even though they're not used, they're still depended upon.
        bool operator==(const VideoCaptureDeviceDescriptor& other) const {
            return (other.device_id == device_id) && (other.capture_api == capture_api);
        }
        bool operator<(const VideoCaptureDeviceDescriptor& other) const;

        const char* GetCaptureApiTypeString() const;
        // Friendly name of a device, plus the model identifier in parentheses.
        std::string GetNameAndModel() const;

        // Name that is intended for display in the UI.
        const std::string& display_name() const { return display_name_; }
        void set_display_name(const std::string& name);

        const VideoCaptureControlSupport& control_support() const {
            return control_support_;
        }
        void set_control_support(const VideoCaptureControlSupport& control_support) {
            control_support_ = control_support;
        }

        std::string device_id;
        // A unique hardware identifier of the capture device.
        // It is of the form "[vid]:[pid]" when a USB device is detected, and empty
        // otherwise.
        std::string model_id;

        VideoFacingMode facing;

        VideoCaptureApi capture_api;
        VideoCaptureTransportType transport_type;

    private:
        std::string display_name_;  // Name that is intended for display in the UI
        VideoCaptureControlSupport control_support_;
    };

    using VideoCaptureDeviceDescriptors = std::vector<VideoCaptureDeviceDescriptor>;

    class Client {
    public:
        virtual void OnStarted() = 0;
        virtual void OnIncomingCapturedData(const uint8_t* data,
            int length, const VideoCaptureFormat& frame_format) = 0;
        virtual void OnFrameDropped(VideoCaptureFrameDropReason reason) = 0;
        virtual void OnError(VideoCaptureError error,
            const std::string& reason) = 0;
    };

    // Simple scoped memory releaser class for COM allocated memory.
// Example:
//   base::win::ScopedCoMem<ITEMIDLIST> file_item;
//   SHGetSomeInfo(&file_item, ...);
//   ...
//   return;  <-- memory released
    template <typename T>
    class ScopedCoMem {
    public:
        ScopedCoMem() : mem_ptr_(nullptr) {}
        ~ScopedCoMem() { Reset(nullptr); }

        T** operator&() {               // NOLINT
            DCHECK(mem_ptr_ == nullptr);  // To catch memory leaks.
            return &mem_ptr_;
        }

        operator T* () { return mem_ptr_; }

        T* operator->() {
            DCHECK(mem_ptr_ != NULL);
            return mem_ptr_;
        }

        const T* operator->() const {
            DCHECK(mem_ptr_ != NULL);
            return mem_ptr_;
        }

        void Reset(T* ptr) {
            if (mem_ptr_)
                CoTaskMemFree(mem_ptr_);
            mem_ptr_ = ptr;
        }

        T* get() const { return mem_ptr_; }

    private:
        T* mem_ptr_;

    };

    const REFERENCE_TIME kSecondsToReferenceTime = 10000000;

    // Define GUID for I420. This is the color format we would like to support but
    // it is not defined in the DirectShow SDK.
    // http://msdn.microsoft.com/en-us/library/dd757532.aspx
    // 30323449-0000-0010-8000-00AA00389B71.
    const GUID kMediaSubTypeI420 = {
        0x30323449,
        0x0000,
        0x0010,
        {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71} };

    // UYVY synonym with BT709 color components, used in HD video. This variation
    // might appear in non-USB capture cards and it's implemented as a normal YUV
    // pixel format with the characters HDYC encoded in the first array word.
    const GUID kMediaSubTypeHDYC = {
        0x43594448,
        0x0000,
        0x0010,
        {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };

    // 16-bit grey-scale single plane formats provided by some depth cameras.
    const GUID kMediaSubTypeZ16 = {
        0x2036315a,
        0x0000,
        0x0010,
        {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };
    const GUID kMediaSubTypeINVZ = {
        0x5a564e49,
        0x2d90,
        0x4a58,
        {0x92, 0x0b, 0x77, 0x3f, 0x1f, 0x2c, 0x55, 0x6b} };
    const GUID kMediaSubTypeY16 = {
        0x20363159,
        0x0000,
        0x0010,
        {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };

    std::wstring WStringFromGUID(REFGUID rguid);

    
    const CapabilityWin& GetBestMatchedCapability(
        const VideoCaptureFormat& requested,
        const CapabilityList& capabilities);


    class SinkFilterObserver {
    public:
        // SinkFilter will call this function with all frames delivered to it.
        // |buffer| is only valid during this function call.
        virtual void FrameReceived(const uint8_t* buffer,
            int length,
            const VideoCaptureFormat& format,
            long timestamp,
            bool flip_y) = 0;

        virtual void FrameDropped(VideoCaptureFrameDropReason reason) = 0;

    protected:
        virtual ~SinkFilterObserver();
    };


    class FilterBase : public IBaseFilter{
    public:
        FilterBase();

        // Number of pins connected to this filter.
        virtual size_t NoOfPins() = 0;
        // Returns the IPin interface pin no index.
        virtual IPin* GetPin(int index) = 0;

        // Inherited from IUnknown.
        IFACEMETHODIMP QueryInterface(REFIID id, void** object_ptr) override;
        IFACEMETHODIMP_(ULONG) AddRef() override;
        IFACEMETHODIMP_(ULONG) Release() override;

        // Inherited from IBaseFilter.
        IFACEMETHODIMP EnumPins(IEnumPins** enum_pins) override;

        IFACEMETHODIMP FindPin(LPCWSTR id, IPin** pin) override;

        IFACEMETHODIMP QueryFilterInfo(FILTER_INFO* info) override;

        IFACEMETHODIMP JoinFilterGraph(IFilterGraph* graph, LPCWSTR name) override;

        IFACEMETHODIMP QueryVendorInfo(LPWSTR* vendor_info) override;

        // Inherited from IMediaFilter.
        IFACEMETHODIMP Stop() override;

        IFACEMETHODIMP Pause() override;

        IFACEMETHODIMP Run(REFERENCE_TIME start) override;

        IFACEMETHODIMP GetState(DWORD msec_timeout, FILTER_STATE* state) override;

        IFACEMETHODIMP SetSyncSource(IReferenceClock* clock) override;

        IFACEMETHODIMP GetSyncSource(IReferenceClock** clock) override;

        // Inherited from IPersistent.
        IFACEMETHODIMP GetClassID(CLSID* class_id) override = 0;

    protected:
        virtual ~FilterBase();

    private:
        FILTER_STATE state_;
        Microsoft::WRL::ComPtr<IFilterGraph> owning_graph_;
    };


    class SinkInputPin;

    class PinBase : public IPin,
        public IMemInputPin {
    public:
        explicit PinBase(IBaseFilter* owner);

        // Function used for changing the owner.
        // If the owner is deleted the owner should first call this function
        // with owner = NULL.
        void SetOwner(IBaseFilter* owner);

        // Checks if a media type is acceptable. This is called when this pin is
        // connected to an output pin. Must return true if the media type is
        // acceptable, false otherwise.
        virtual bool IsMediaTypeValid(const AM_MEDIA_TYPE* media_type) = 0;

        // Enumerates valid media types.
        virtual bool GetValidMediaType(int index, AM_MEDIA_TYPE* media_type) = 0;

        // Called when new media is received. Note that this is not on the same
        // thread as where the pin is created.
        IFACEMETHODIMP Receive(IMediaSample* sample) override = 0;

        IFACEMETHODIMP Connect(IPin* receive_pin,
            const AM_MEDIA_TYPE* media_type) override;

        IFACEMETHODIMP ReceiveConnection(IPin* connector,
            const AM_MEDIA_TYPE* media_type) override;

        IFACEMETHODIMP Disconnect() override;

        IFACEMETHODIMP ConnectedTo(IPin** pin) override;

        IFACEMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE* media_type) override;

        IFACEMETHODIMP QueryPinInfo(PIN_INFO* info) override;

        IFACEMETHODIMP QueryDirection(PIN_DIRECTION* pin_dir) override;

        IFACEMETHODIMP QueryId(LPWSTR* id) override;

        IFACEMETHODIMP QueryAccept(const AM_MEDIA_TYPE* media_type) override;

        IFACEMETHODIMP EnumMediaTypes(IEnumMediaTypes** types) override;

        IFACEMETHODIMP QueryInternalConnections(IPin** pins, ULONG* no_pins) override;

        IFACEMETHODIMP EndOfStream() override;

        IFACEMETHODIMP BeginFlush() override;

        IFACEMETHODIMP EndFlush() override;

        IFACEMETHODIMP NewSegment(REFERENCE_TIME start,
            REFERENCE_TIME stop,
            double dRate) override;

        // Inherited from IMemInputPin.
        IFACEMETHODIMP GetAllocator(IMemAllocator** allocator) override;

        IFACEMETHODIMP NotifyAllocator(IMemAllocator* allocator,
            BOOL read_only) override;

        IFACEMETHODIMP GetAllocatorRequirements(
            ALLOCATOR_PROPERTIES* properties) override;

        IFACEMETHODIMP ReceiveMultiple(IMediaSample** samples,
            long sample_count,
            long* processed) override;
        IFACEMETHODIMP ReceiveCanBlock() override;

        // Inherited from IUnknown.
        IFACEMETHODIMP QueryInterface(REFIID id, void** object_ptr) override;

        IFACEMETHODIMP_(ULONG) AddRef() override;

        IFACEMETHODIMP_(ULONG) Release() override;

    protected:
        virtual ~PinBase();

    private:
        AM_MEDIA_TYPE current_media_type_;
        Microsoft::WRL::ComPtr<IPin> connected_pin_;
        // owner_ is the filter owning this pin. We don't reference count it since
        // that would create a circular reference count.
        IBaseFilter* owner_;
    };

    // Input pin of the SinkFilter.
    class SinkInputPin : public PinBase {
    public:
        SinkInputPin(IBaseFilter* filter, SinkFilterObserver* observer);

        void SetRequestedMediaFormat(VideoPixelFormat pixel_format,
            float frame_rate,
            const BITMAPINFOHEADER& info_header);

        // Implement PinBase.
        bool IsMediaTypeValid(const AM_MEDIA_TYPE* media_type) override;
        bool GetValidMediaType(int index, AM_MEDIA_TYPE* media_type) override;

        IFACEMETHODIMP Receive(IMediaSample* media_sample) override;

    private:
        ~SinkInputPin() override;

        VideoPixelFormat requested_pixel_format_;
        float requested_frame_rate_;
        BITMAPINFOHEADER requested_info_header_;
        VideoCaptureFormat resulting_format_;
        bool flip_y_;
        SinkFilterObserver* observer_;
    };

    class __declspec(uuid("88cdbbdc-a73b-4afa-acbf-15d5e2ce12c3")) SinkFilter
        : public FilterBase {
    public:
        explicit SinkFilter(SinkFilterObserver* observer);

        void SetRequestedMediaFormat(VideoPixelFormat pixel_format,
            float frame_rate,
            const BITMAPINFOHEADER& info_header);

        // Implement FilterBase.
        size_t NoOfPins() override;
        IPin* GetPin(int index) override;

        IFACEMETHODIMP GetClassID(CLSID* clsid) override;

    private:
        ~SinkFilter() override;

        Microsoft::WRL::ComPtr<SinkInputPin> input_pin_;

    };




}