#include "kconfigraweditor.h"

#include <unistd.h>

#include <QFile>
#include <QTextStream>

#include <krandom.h>
#include <kstandarddirs.h>

#ifndef KCONFIGRAWEDITORTEST
#include "kioskrun.h"
#endif

KConfigRawEditor::KConfigEntryData::KConfigEntryData( const QString &_value, DataType _type )
{
    value = _value;
    type = _type;
}

KConfigRawEditor::KConfigRawEditor(const QString& fileName, const QString& system )
{
    m_configFile = fileName;
    Q_UNUSED(system); // TODO: handle other KConfig backends later.
}

KConfigRawEditor::~KConfigRawEditor()
{
    qDeleteAll( m_configData );
    m_configData.clear();
}

void KConfigRawEditor::setMutable( bool isMutable )
{
    m_isMutable = isMutable;
}

bool KConfigRawEditor::isMutable( ) const
{
    return m_isMutable;
}

bool KConfigRawEditor::load()
{
    QFile configFile( m_configFile );
    if( configFile.open( QIODevice::ReadOnly ) )
    {
	QString currentGroup;
	while( !configFile.atEnd() )
	{
            QString line = configFile.readLine();

	    if( line.isEmpty() )
		continue;
	    else if( line.startsWith("[") )
	    {
		if( line.startsWith("[$i]") )
		{
		    if( currentGroup.isEmpty() )
			setMutable( true );
		    else
			group(currentGroup)->isMutable = false;
		}
		else
		{
		    currentGroup = line.mid( 1, line.indexOf( "]") - 1 );
		    if( line.contains("[$i]") )
			group(currentGroup)->isMutable = false;
		    else
		        group(currentGroup)->isMutable = true;
		}

	    }
	    else // we are a value
	    {
		KConfigEntryData entry;
		if( line.contains( "[$i]" ) )
		    entry.type = KConfigEntryData::Immutable;
		else if ( line.contains( "[$e]" ) )
		    entry.type = KConfigEntryData::Expandable;
		else if ( line.contains( "[$ie]" ) || line.contains("[$ei]" ) )
		    entry.type = KConfigEntryData::ImmutableAndExpandable;
		else if ( line.contains( "[$d]" ) )
		    entry.type = KConfigEntryData::Deleted;
		else if ( line.contains( "[$id]" ) || line.contains("[$di]" ) )
		    entry.type = KConfigEntryData::ImmutableAndDeleted;

                entry.value = line.mid( line.indexOf('=') + 1 ).trimmed();
                QString key = line.left( line.indexOf('=') ).trimmed();

		key.remove("[$i]");
		key.remove("[$e]");
		key.remove("[$ie]");
		key.remove("[$ei]");
		key.remove("[$d]");
		key.remove("[$id]");
		key.remove("[$di]");

		group(currentGroup)->data[key] = entry;
	    }
	}
    }
    else
	return false;

    return true;
}

bool KConfigRawEditor::save()
{
#ifdef KCONFIGRAWEDITORTEST
    QFile configFile( m_configFile );
#else
    QString localFile = ::KStandardDirs::locateLocal("tmp", "kiosktoolconfigfile_"+KRandom::randomString(5));
    ::unlink(QFile::encodeName(localFile));
    QFile configFile( localFile );
#endif

    if( configFile.open( QIODevice::WriteOnly ) )
    {
	QTextStream ts( &configFile );
	foreach( QString groupName, m_configData.keys() )
	{
	    if( !group(groupName)->data.isEmpty() )
	    {
		ts << "[" << groupName << "]\n";
		if( !group(groupName)->isMutable )
		    ts << "[$i]\n";
		foreach( QString entry, group(groupName)->data.keys() )
		{
		    if( group(groupName)->data[entry].value.isEmpty() )
			continue;
                    ts << entry.trimmed();
		    switch( group(groupName)->data[entry].type )
		    {
			case KConfigEntryData::Normal:
			    ts << "=";
			break;
			case KConfigEntryData::Immutable:
			    ts << "[$i]=";
			break;
			case KConfigEntryData::Expandable:
			    ts << "[$e]=";
			break;
			case KConfigEntryData::ImmutableAndExpandable:
			    ts << "[$ie]=";
			break;
			case KConfigEntryData::Deleted:
			    ts << "[$d]=";
			break;
			case KConfigEntryData::ImmutableAndDeleted:
			    ts << "[$id]=";
			break;
		    };
                    if( group(groupName)->data[entry].type != KConfigEntryData::ImmutableAndDeleted &&
                        group(groupName)->data[entry].type != KConfigEntryData::Deleted )
                            ts << group(groupName)->data[entry].value << "\n";
                    else
                        ts << "\n";
		}
	    }
	}
    }
    else
	return false;

    configFile.close();
#ifndef KCONFIGRAWEDITORTEST
    // install the file to the right location with the right owner
    if( !KioskRun::self()->install( localFile, m_configFile ))
        return false;
#endif

    return true;
}

KConfigRawEditor::KConfigGroupData *KConfigRawEditor::group( const QString &groupName ) const
{
    KConfigGroupData *groupData = m_configData[ groupName ];
    if( groupData )
	return groupData;
    else
    {
	groupData = new KConfigGroupData;
	groupData->isMutable = true;
	m_configData[ groupName ] = groupData;
	return groupData;
    }
}

QVariant KConfigRawEditor::KConfigGroupData::readEntry( const QString &key, const QVariant &defaultValue ) const
{

    if( data[key].value.isEmpty() )
	return defaultValue;
    else
    {
	QVariant returnValue( data[key].value );
	returnValue.convert( defaultValue.type() );
	return returnValue;
    }
}

void KConfigRawEditor::KConfigGroupData::writeEntry( const QString &key, const QVariant &value, KConfigEntryData::DataType type )
{
    data[key].value = value.toString();
    data[key].type = type;
}

bool KConfigRawEditor::KConfigGroupData::hasKey( const QString &key ) const
{
    return data.contains(key);
}

void KConfigRawEditor::KConfigGroupData::deleteEntry( const QString &key, KConfigEntryData::DataType type )
{
    data.remove(key);
    if ( type == KConfigEntryData::Immutable )
    {
        data[key].type = KConfigEntryData::ImmutableAndDeleted;
    }
}
