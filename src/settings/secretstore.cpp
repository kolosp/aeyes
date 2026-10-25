#include "secretstore.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QEventLoop>
#include <QTimer>
#include <qt6keychain/keychain.h>

static const QString service = QStringLiteral("aeyes");

SecretStore::SecretStore(QObject *parent)
    : QObject(parent)
{
}

static QString envFor(const QString &provider)
{
    return qEnvironmentVariable(provider == QLatin1String("gemini") ? "GEMINI_API_KEY" : "OPENAI_API_KEY");
}

// "A key was saved for this provider" is remembered in aeyesrc, so we never open the wallet
// (and make the user type its password) just to find out that there is nothing in it.
static KConfigGroup flags()
{
    return KConfigGroup(KSharedConfig::openConfig(QStringLiteral("aeyesrc")), QStringLiteral("Secrets"));
}

static bool stored(const QString &provider)
{
    const KConfigGroup g = flags();
    if (g.hasKey(QStringLiteral("stored-") + provider))
        return g.readEntry(QStringLiteral("stored-") + provider, false);
    // No record yet. A config that already went through onboarding predates the flag, so its key may
    // be in the wallet: look once and record the answer. A brand-new config has nothing to look for.
    const KConfigGroup general(KSharedConfig::openConfig(QStringLiteral("aeyesrc")), QStringLiteral("General"));
    return !general.readEntry("ackedProviders", QStringList()).isEmpty();
}

void SecretStore::load(const QString &provider)
{
    if (m_loaded.value(provider))
        return;
    if (!stored(provider)) {
        m_loaded[provider] = true;
        return;
    }
    auto *job = new QKeychain::ReadPasswordJob(service, this);
    job->setKey(provider);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(5000);
    job->start();
    loop.exec();
    if (job->error() == QKeychain::NoError) {
        m_keys[provider] = job->textData();
        flags().writeEntry(QStringLiteral("stored-") + provider, !job->textData().isEmpty());
        flags().sync();
    } else if (job->error() == QKeychain::EntryNotFound) {
        flags().writeEntry(QStringLiteral("stored-") + provider, false);
        flags().sync();
    } else {
        m_walletOk = false; // wallet locked/denied/unavailable: don't record anything, ask again next time
    }
    m_loaded[provider] = true;
    job->deleteLater();
}

QString SecretStore::key(const QString &provider)
{
    const QString env = envFor(provider);
    if (!env.isEmpty())
        return env;
    load(provider);
    return m_keys.value(provider);
}

bool SecretStore::hasKey(const QString &provider)
{
    return !key(provider).isEmpty();
}

void SecretStore::setKey(const QString &provider, const QString &value)
{
    m_keys[provider] = value;
    m_loaded[provider] = true;
    flags().writeEntry(QStringLiteral("stored-") + provider, !value.isEmpty());
    flags().sync();
    if (value.isEmpty()) {
        auto *job = new QKeychain::DeletePasswordJob(service, this);
        job->setKey(provider);
        connect(job, &QKeychain::Job::finished, job, &QObject::deleteLater);
        job->start();
    } else {
        auto *job = new QKeychain::WritePasswordJob(service, this);
        job->setKey(provider);
        job->setTextData(value);
        connect(job, &QKeychain::Job::finished, this, [this, job] {
            if (job->error() != QKeychain::NoError) {
                m_walletOk = false;
                Q_EMIT changed();
            }
            job->deleteLater();
        });
        job->start();
    }
    Q_EMIT changed();
}
