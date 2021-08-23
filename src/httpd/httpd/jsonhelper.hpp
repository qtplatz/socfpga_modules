// -*- C++ -*-
/**************************************************************************
** Copyright (C) 2021 Toshinobu Hondo, Ph.D.
** Copyright (C) 2021 MS-Cheminformatics LLC
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid ScienceLiaison commercial licenses may use this
** file in accordance with the ScienceLiaison Commercial License Agreement
** provided with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and ScienceLiaison.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.TXT included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/

#pragma once

#include <boost/json.hpp>
#include <optional>

struct jsonhelper {
    static std::optional< boost::json::value > get_optional( boost::json::value& jv, const std::string& key ) {
        if ( jv.kind() == boost::json::kind::object && jv.as_object().contains( key ) )
            return jv.as_object()[ key ];
        return {};
    }
    template< typename T > static T to_ ( boost::json::value& jv ) {
        if ( jv.kind() == boost::json::kind::string ) {
            if ( typeid(T) == typeid( double ) || typeid(T) == typeid( float ) )
                return atof( boost::json::value_to< std::string >( jv ).c_str() );
            else
                return std::stol( boost::json::value_to< std::string >( jv ).c_str() );
        }
        return boost::json::value_to< T >( jv );
    }
};
