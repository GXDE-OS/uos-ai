#include "stringgenerator.h"

StringGenerator::StringGenerator()
    : m_currentString("a")
{

}

void StringGenerator::incrementString()
{
    int length = m_currentString.length();
    bool carry = true;

    for (int i = length - 1; i >= 0 && carry; --i) {
        QChar currentChar = m_currentString.at(i);
        QChar nextChar = currentChar;

        if (currentChar == 'z') {
            nextChar = 'a';
        } else {
            nextChar = currentChar.unicode() + 1;
            carry = false;
        }

        m_currentString[i] = nextChar;
    }

    if (carry) {
        m_currentString = 'a' + m_currentString;
    }
}

QString StringGenerator::generateNextString()
{
    QString result = m_currentString;

    incrementString();

    return result;
}
