// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAMERA_H
#define QAVFCAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qavfcamerabase_p.h"
#include <private/qplatformmediaintegration_p.h>
#include <private/qvideooutputorientationhandler_p.h>
#define AVMediaType XAVMediaType
#include "qffmpeghwaccel_p.h"
#undef AVMediaType

#include <qfilesystemwatcher.h>
#include <qsocketnotifier.h>
#include <qmutex.h>

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureSession);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceInput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoDataOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(QAVFSampleBufferDelegate);

QT_BEGIN_NAMESPACE

class QFFmpegVideoSink;

class QAVFCamera : public QAVFCameraBase
{
    Q_OBJECT

public:
    explicit QAVFCamera(QCamera *parent);
    ~QAVFCamera();

    bool isActive() const override;
    void setActive(bool active) override;

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    void syncHandleFrame(const QVideoFrame &frame);

    void deviceOrientationChanged(int angle = -1);

    const void *ffmpegHWAccel() const override { return &hwAccel; }

private:
    void requestCameraPermissionIfNeeded();
    void cameraAuthorizationChanged(bool authorized);
    void updateCameraFormat();
    void updateVideoInput();
    void attachVideoInputDevice();
    uint setPixelFormat(const QVideoFrameFormat::PixelFormat pixelFormat);

    AVCaptureDevice *device() const;

    QMediaCaptureSession *m_session = nullptr;
    AVCaptureSession *m_captureSession = nullptr;
    AVCaptureDeviceInput *m_videoInput = nullptr;
    AVCaptureVideoDataOutput *m_videoDataOutput = nullptr;
    QAVFSampleBufferDelegate *m_sampleBufferDelegate = nullptr;
    dispatch_queue_t m_delegateQueue;
    QVideoOutputOrientationHandler m_orientationHandler;
    QFFmpeg::HWAccel hwAccel;
};

QT_END_NAMESPACE


#endif  // QFFMPEGCAMERA_H

