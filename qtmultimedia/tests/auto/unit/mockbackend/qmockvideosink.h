// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMOCKVIDEOSINK_H
#define QMOCKVIDEOSINK_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QMockVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    explicit QMockVideoSink(QVideoSink *parent)
        : QPlatformVideoSink(parent)
    {}
    void setRhi(QRhi * /*rhi*/) override {}
};

QT_END_NAMESPACE

#endif
