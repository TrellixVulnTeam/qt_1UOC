// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TEST262RUNNER_H
#define TEST262RUNNER_H
#include <qstring.h>
#include <qstringlist.h>
#include <qset.h>
#include <qmap.h>
#include <qmutex.h>
#include <qthreadpool.h>

struct TestCase {
    TestCase() = default;
    TestCase(const QString &test)
        : test(test) {}
    enum Result {
        Skipped,
        Passes,
        Fails,
        Crashes
    };
    bool skipTestCase = false;
    Result strictExpectation = Passes;
    Result sloppyExpectation = Passes;
    Result strictResult = Skipped;
    Result sloppyResult = Skipped;

    QString test;
};

struct TestData : TestCase {
    TestData(const TestCase &testCase)
        : TestCase(testCase) {}
    // flags
    bool negative = false;
    bool runInStrictMode = true;
    bool runInSloppyMode = true;
    bool runAsModuleCode = false;
    bool async = false;

    bool isExcluded = false;

    QList<QByteArray> includes;

    QByteArray harness;
    QByteArray content;
};

class Test262Runner
{
public:
    Test262Runner(const QString &command, const QString &testDir);
    ~Test262Runner();

    enum Mode {
        Sloppy = 0,
        Strict = 1
    };

    enum Flags {
        Verbose = 0x1,
        Parallel = 0x2,
        ForceBytecode = 0x4,
        ForceJIT = 0x8,
        WithTestExpectations = 0x10,
        UpdateTestExpectations = 0x20,
        WriteTestExpectations = 0x40,
    };
    void setFlags(int f) { flags = f; }

    void setFilter(const QString &f) { filter = f; }

    void cat();
    bool run();

    bool report();

private:
    friend class SingleTest;
    bool loadTests();
    void loadTestExpectations();
    void updateTestExpectations();
    void writeTestExpectations();
    int runSingleTest(TestCase testCase);

    TestData getTestData(const TestCase &testCase);
    void parseYaml(const QByteArray &content, TestData *data);

    QByteArray harness(const QByteArray &name);

    void addResult(TestCase result);

    QString command;
    QString testDir;
    int flags = 0;

    QMutex mutex;
    QString filter;

    QMap<QString, TestCase> testCases;
    QHash<QByteArray, QByteArray> harnessFiles;

    QThreadPool *threadPool = nullptr;
};


#endif
