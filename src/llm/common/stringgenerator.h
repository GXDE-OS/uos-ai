#ifndef STRINGGENERATOR_H
#define STRINGGENERATOR_H

#include <QString>

class StringGenerator
{
public:
    StringGenerator();
    QString generateNextString();

private:
    void incrementString();

private:
    QString m_currentString;
};

#endif // STRINGGENERATOR_H
