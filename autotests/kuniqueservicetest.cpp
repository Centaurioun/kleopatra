/* This file is part of Kleopatra

   SPDX-FileCopyrightText: 2016 Intevation GmbH

   It is based on libkdbus kdbusservicetest which is:

   SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
   SPDX-FileCopyrightText: 2011 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2011 Kevin Ottens <ervin@kde.org>

   SPDX-License-Identifier: LGPL-2.0-only
*/

/* The main modification in this test is that every activateRequested
 * call needs to set the exit code to signal the application it's done. */

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QProcess>
#include <QTimer>

#include "utils/kuniqueservice.h"

#include <stdio.h>
#include <chrono>
using namespace std::chrono_literals;

class TestObject : public QObject
{
    Q_OBJECT
public:
    TestObject(KUniqueService *service)
        : m_proc(nullptr), m_callCount(0),
          m_service(service)
    {}

    ~TestObject() override
    {
        if (m_proc) {
            m_proc->waitForFinished();
        }
    }

    int callCount() const
    {
        return m_callCount;
    }

private Q_SLOTS:
    void slotActivateRequested(const QStringList &args, const QString &workingDirectory)
    {
        Q_UNUSED(workingDirectory)
        qDebug() << "Application executed with args" << args;

        ++m_callCount;

        if (m_callCount == 1) {
            Q_ASSERT(args.count() == 1);
            Q_ASSERT(args.at(0) == QLatin1String("dummy call"));
            m_service->setExitValue(0);
        } else if (m_callCount == 2) {
            Q_ASSERT(args.count() == 2);
            Q_ASSERT(args.at(1) == QLatin1String("bad call"));
            m_service->setExitValue(4);
        } else if (m_callCount == 3) {
            Q_ASSERT(args.count() == 3);
            Q_ASSERT(args.at(1) == QLatin1String("real call"));
            Q_ASSERT(args.at(2) == QLatin1String("second arg"));
            m_service->setExitValue(0);
            // OK, all done, quit
            QCoreApplication::instance()->quit();
        }
    }

    void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        Q_UNUSED(exitStatus)
        qDebug() << "Process exited with code" << exitCode;
        m_proc = nullptr;
        if (m_callCount == 2) {
            Q_ASSERT(exitCode == 4);
            secondCall();
        }
    }

    void firstCall()
    {
        QStringList args;
        args << QStringLiteral("bad call");
        executeNewChild(args);
    }

    void secondCall()
    {
        QStringList args;
        args << QStringLiteral("real call") << QStringLiteral("second arg");
        executeNewChild(args);
    }

private:
    void executeNewChild(const QStringList &args)
    {
        // Duplicated from kglobalsettingstest.cpp - make a shared helper method?
        m_proc = new QProcess(this);
        connect(m_proc, SIGNAL(finished(int,QProcess::ExitStatus)),
                this, SLOT(slotProcessFinished(int,QProcess::ExitStatus)));
        QString appName = QStringLiteral("kuniqueservicetest");
#ifdef Q_OS_WIN
        appName += QStringLiteral(".exe");
#else
        if (QFile::exists(appName + QStringLiteral(".shell"))) {
            appName = QStringLiteral("./") + appName + QStringLiteral(".shell");
        } else if (QFile::exists(QCoreApplication::applicationFilePath())) {
            appName = QCoreApplication::applicationFilePath();
        } else {
            Q_ASSERT(QFile::exists(appName));
            appName = QStringLiteral("./") + appName;
        }
#endif
        qDebug() << "about to run" << appName << args;
        m_proc->start(appName, args);
    }

    QProcess *m_proc;
    int m_callCount;
    KUniqueService *m_service;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("kuniqueservicetest"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));

    KUniqueService service;
    TestObject testObject(&service);
    QObject::connect(&service, SIGNAL(activateRequested(QStringList,QString)),
                     &testObject, SLOT(slotActivateRequested(QStringList,QString)));

    // Testcase for the problem coming from the old fork-on-startup solution:
    // the "Activate" D-Bus call would time out if the app took too much time
    // to be ready.
    //printf("Sleeping.\n");
    //sleep(200);
    QStringList args;
    args << QStringLiteral("dummy call");

    QMetaObject::invokeMethod(&service, "activateRequested",
                              Qt::QueuedConnection,
                              Q_ARG(QStringList, args),
                              Q_ARG(QString, QDir::currentPath()));
    QTimer::singleShot(400ms, &testObject, SLOT(firstCall()));

    qDebug() << "Running.";
    a.exec();
    qDebug() << "Terminating.";

    Q_ASSERT(testObject.callCount() == 3);
    const bool ok = testObject.callCount() == 3;

    return ok ? 0 : 1;
}

#include "kuniqueservicetest.moc"

