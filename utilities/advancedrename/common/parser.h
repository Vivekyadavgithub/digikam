/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2009-08-11
 * Description : the main parser object for the AdvancedRename utility
 *
 * Copyright (C) 2009 by Andi Clemens <andi dot clemens at gmx dot net>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef PARSER_H
#define PARSER_H

// Qt includes

#include <QList>
#include <QMap>
#include <QString>

// Local includes

#include "parsesettings.h"
#include "parseresults.h"
#include "option.h"
#include "modifier.h"

namespace Digikam
{

class Option;
class Modifier;
class ParserPriv;

class Parser
{

public:

    enum TokenType
    {
        OptionToken = 0,
        OptionModifiersToken,
        TextToken
    };

public:

    Parser();
    virtual ~Parser();

    void          init(const ParseSettings& settings);
    void          reset();

    QString       parse(ParseSettings& settings);

    OptionsList   options()   const;
    ModifierList  modifiers() const;

    bool          tokenAtPosition(TokenType type, ParseSettings& settings, int pos);
    bool          tokenAtPosition(TokenType type, ParseSettings& settings, int pos, int& start, int& length);

    /**
     * check if the given parse string is valid
     * @param str the parse string
     * @return true if valid / can be parsed
     */
    static bool parseStringIsValid(const QString& str);

protected:

    void registerOption(Option* option);
    void registerModifier(Modifier* modifier);

private:

    ParseResults results(ParseSettings& settings, bool modify = true);
    QString      parseOperation(ParseSettings& settings, ParseResults& results, bool modify = true);
    void         applyModifiers(const QString& parseString, ParseResults& results);

private:

    ParserPriv* const d;
};

}  // namespace Digikam

#endif /* PARSER_H */
