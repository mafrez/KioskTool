#ifndef KCONFIGRAWEDITOR_H
#define KCONFIGRAWEDITOR_H

#include <QHash>
#include <QVariant>

class KConfigRawEditor
{
public:

    struct KConfigEntryData {
	enum DataType { Normal, Immutable, Expandable, ImmutableAndExpandable, Deleted, ImmutableAndDeleted };
	KConfigEntryData( const QString &_value, DataType _type = Normal );
	KConfigEntryData() { type = Normal; }
	QString value;
	DataType type;
    };

    struct KConfigGroupData {
	QVariant readEntry( const QString &key, const QVariant &defaultValue = QVariant() ) const;
        void writeEntry( const QString &key, const QVariant &value, KConfigEntryData::DataType type = KConfigEntryData::Normal );
	bool hasKey( const QString &key ) const;
        void deleteEntry( const QString &key, KConfigEntryData::DataType type = KConfigEntryData::Normal  );

	QHash<QString, KConfigEntryData> data;
	bool isMutable;
    };

    KConfigRawEditor(const QString& fileName, const QString& system = QLatin1String("INI") );
    ~KConfigRawEditor();

    bool load();
    bool save();

    KConfigGroupData *group( const QString &groupName ) const;
    void setMutable( bool isMutable );
    bool isMutable( ) const;

private:
    mutable QHash< QString, KConfigGroupData *> m_configData;
    QString m_configFile;
    bool m_isMutable;
};


#endif // KCONFIGRAWEDITOR_H
