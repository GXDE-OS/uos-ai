#include "updateversion.h"

namespace uos_ai {

QString UpdateVersion::stripPackageRevision(const QString &version)
{
    return version.trimmed().section(QLatin1Char('-'), 0, 0).trimmed();
}

QVersionNumber UpdateVersion::parseNumber(const QString &version)
{
    const QString trimmedVersion = stripPackageRevision(version);
    if (trimmedVersion.isEmpty()) {
        return {};
    }

    QVersionNumber parsedVersion = QVersionNumber::fromString(trimmedVersion);
    if (parsedVersion.isNull()) {
        return {};
    }

    return parsedVersion;
}

QString UpdateVersion::normalize(const QString &version)
{
    const QVersionNumber parsedVersion = parseNumber(version);
    if (parsedVersion.isNull()) {
        return {};
    }

    return parsedVersion.toString();
}

int UpdateVersion::compare(const QString &left, const QString &right)
{
    const QVersionNumber leftVersion = parseNumber(left);
    const QVersionNumber rightVersion = parseNumber(right);
    if (leftVersion.isNull() || rightVersion.isNull()) {
        return 0;
    }

    return QVersionNumber::compare(leftVersion, rightVersion);
}

}
