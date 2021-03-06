/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _AUDIO_TARGET_FACTORY_H_
#define _AUDIO_TARGET_FACTORY_H_

#include <vector>
#include <QString>

#include "base/Debug.h"

class AudioCallbackPlaySource;
class AudioCallbackPlayTarget;

class AudioTargetFactory 
{
public:
    static AudioTargetFactory *getInstance();

    std::vector<QString> getCallbackTargetNames(bool includeAuto = true) const;
    QString getCallbackTargetDescription(QString name) const;
    QString getDefaultCallbackTarget() const;
    bool isAutoCallbackTarget(QString name) const;
    void setDefaultCallbackTarget(QString name);

    AudioCallbackPlayTarget *createCallbackTarget(AudioCallbackPlaySource *);

protected:
    AudioTargetFactory();
    static AudioTargetFactory *m_instance;
    QString m_default;
};

#endif

