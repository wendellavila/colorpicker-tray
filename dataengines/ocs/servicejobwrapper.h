#ifndef SERVICEJOBWRAPPER_H
#define SERVICEJOBWRAPPER_H

#include <Plasma/ServiceJob>


namespace Attica {
    class BaseJob;
}

class ServiceJobWrapper : public Plasma::ServiceJob
{
    Q_OBJECT

    public:
        ServiceJobWrapper(Attica::BaseJob* job, const QString& destination, const QString& operation, const QMap<QString, QVariant>& parameters, QObject* parent = 0);
        void start();

    private Q_SLOTS:
        void finished(Attica::BaseJob* job);

    private:
        Attica::BaseJob* m_job;
};

class CredentialsJob : public Plasma::ServiceJob
{
    Q_OBJECT
public:
    CredentialsJob(const QString &destination, const QString &operation, const QMap< QString, QVariant > &parameters, QObject *parent=0);
    void start();
};

#endif
