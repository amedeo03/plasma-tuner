#include "core/instrumentconfig.h"
#include "core/notes.h"

#include <QTest>

using namespace InstrumentConfig;

namespace
{
// JSON fixtures are written with single quotes, to avoid both escaping and raw
// string literals (which moc fails to parse).
QByteArray json(const char *text)
{
    return QByteArray(text).replace('\'', '"');
}

QString notesString(const Tuning &tuning)
{
    QStringList names;
    for (const int midi : tuning.notes) {
        names.append(Notes::fullName(midi));
    }
    return names.join(QLatin1Char(' '));
}

// A one-instrument file around the given instrument JSON.
Config parseInstrument(const QByteArray &instrument)
{
    return parse("{\"instruments\": [" + instrument + "]}");
}

// A one-tuning, 3-string instrument around the given notes JSON.
Tuning parseNotes(const QByteArray &notes)
{
    const Config config = parseInstrument(json("{'name': 'Test', 'stringCount': 3, 'tunings': [{'name': 'T', 'stringNotes': ") + notes + "}]}");
    return config.instruments.value(0).tunings.value(0);
}
}

class InstrumentConfigTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDefaultsAreValid()
    {
        const Config config = parse(defaultJson());
        QVERIFY2(config.error.isEmpty(), qPrintable(config.error));
        QCOMPARE(config.instruments.size(), 6);
        for (const Instrument &instrument : config.instruments) {
            QVERIFY2(instrument.isValid(), qPrintable(instrument.name + u": " + instrument.error));
            QCOMPARE(instrument.tunings.size(), 5);
            for (const Tuning &tuning : instrument.tunings) {
                QVERIFY2(tuning.isValid(), qPrintable(instrument.name + u" / " + tuning.name + u": " + tuning.error));
                QCOMPARE(tuning.notes.size(), instrument.stringCount);
            }
        }
    }

    void testDefaultTunings_data()
    {
        QTest::addColumn<int>("instrument");
        QTest::addColumn<int>("tuning");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("notes");

        QTest::newRow("guitar E standard") << 0 << 0 << "E standard" << "E2 A2 D3 G3 B3 E4";
        QTest::newRow("guitar drop D") << 0 << 1 << "Drop D" << "D2 A2 D3 G3 B3 E4";
        QTest::newRow("guitar D standard") << 0 << 2 << "D standard" << "D2 G2 C3 F3 A3 D4";
        QTest::newRow("guitar drop C") << 0 << 3 << "Drop C" << "C2 G2 C3 F3 A3 D4";
        QTest::newRow("guitar C standard") << 0 << 4 << "C standard" << "C2 F2 A♯2 D♯3 G3 C4";
        QTest::newRow("7-string standard") << 1 << 0 << "E standard" << "B1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("7-string drop A") << 1 << 1 << "Drop A" << "A1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("8-string standard") << 2 << 0 << "E standard" << "F♯1 B1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("8-string drop E") << 2 << 1 << "Drop E" << "E1 B1 E2 A2 D3 G3 B3 E4";
        QTest::newRow("bass E standard") << 3 << 0 << "E standard" << "E1 A1 D2 G2";
        QTest::newRow("bass drop D") << 3 << 1 << "Drop D" << "D1 A1 D2 G2";
        QTest::newRow("bass C standard") << 3 << 4 << "C standard" << "C1 F1 A♯1 D♯2";
        QTest::newRow("5-string bass") << 4 << 0 << "E standard" << "B0 E1 A1 D2 G2";
        QTest::newRow("6-string bass") << 5 << 0 << "E standard" << "B0 E1 A1 D2 G2 C3";
        QTest::newRow("6-string bass drop G") << 5 << 3 << "Drop G" << "G0 D1 G1 C2 F2 A♯2";
    }

    void testDefaultTunings()
    {
        QFETCH(int, instrument);
        QFETCH(int, tuning);
        QFETCH(QString, name);
        QFETCH(QString, notes);

        const Config config = parse(defaultJson());
        const Tuning &parsed = config.instruments.at(instrument).tunings.at(tuning);
        QCOMPARE(parsed.name, name);
        QCOMPARE(notesString(parsed), notes);
    }

    void testLabelsKeepSpelling()
    {
        const Tuning tuning = parseNotes(json("['Bb1', 'F#2', 'E3']"));
        QVERIFY2(tuning.isValid(), qPrintable(tuning.error));
        QCOMPARE(tuning.labels, QStringList({QStringLiteral("B♭1"), QStringLiteral("F♯2"), QStringLiteral("E3")}));
        QCOMPARE(tuning.notes, QList<int>({34, 42, 52}));
    }

    void testFileErrors_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected"); // substring of the error

        QTest::newRow("syntax error line") << QByteArray("{\n  \"instruments\": [\n    {,\n  ]\n}") << "line 3";
        QTest::newRow("empty file") << QByteArray("") << "line 1";
        QTest::newRow("root is array") << QByteArray("[]") << "\"instruments\" array";
        QTest::newRow("instruments not array") << json("{'instruments': {}}") << "\"instruments\" array";
        QTest::newRow("no instruments") << json("{'instruments': []}") << "empty";
    }

    void testFileErrors()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);

        const Config config = parse(json);
        QVERIFY(config.instruments.isEmpty());
        QVERIFY2(config.error.contains(expected), qPrintable(config.error));
    }

    void testInstrumentErrors_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("expected"); // substring of the error

        const QByteArray tunings = json("'tunings': [{'name': 'T', 'stringNotes': ['E2']}]");
        QTest::newRow("not an object") << QByteArray("42") << "Instrument 1" << "not a JSON object";
        QTest::newRow("no name") << json("{'stringCount': 1, ") + tunings + "}" << "Instrument 1" << "no \"name\"";
        QTest::newRow("blank name") << json("{'name': '  ', 'stringCount': 1, ") + tunings + "}" << "Instrument 1" << "no \"name\"";
        QTest::newRow("negative strings") << json("{'name': 'Neg', 'stringCount': -3, ") + tunings + "}" << "Neg" << "found -3";
        QTest::newRow("zero strings") << json("{'name': 'Zero', 'stringCount': 0, ") + tunings + "}" << "Zero" << "found 0";
        QTest::newRow("too many strings") << json("{'name': 'Big', 'stringCount': 13, ") + tunings + "}" << "Big" << "found 13";
        QTest::newRow("fractional strings") << json("{'name': 'Frac', 'stringCount': 6.5, ") + tunings + "}" << "Frac" << "found 6.5";
        QTest::newRow("string count as text") << json("{'name': 'Text', 'stringCount': '6', ") + tunings + "}" << "Text" << "found \"6\"";
        QTest::newRow("missing string count") << json("{'name': 'None', ") + tunings + "}" << "None" << "found nothing";
        QTest::newRow("missing tunings") << json("{'name': 'NoTunings', 'stringCount': 1}") << "NoTunings" << "\"tunings\"";
        QTest::newRow("empty tunings") << json("{'name': 'Empty', 'stringCount': 1, 'tunings': []}") << "Empty" << "\"tunings\"";
    }

    void testInstrumentErrors()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, name);
        QFETCH(QString, expected);

        const Config config = parseInstrument(json);
        QVERIFY2(config.error.isEmpty(), qPrintable(config.error));
        QCOMPARE(config.instruments.size(), 1);
        const Instrument &instrument = config.instruments.first();
        QCOMPARE(instrument.name, name);
        QVERIFY(!instrument.isValid());
        QVERIFY2(instrument.error.contains(expected), qPrintable(instrument.error));
    }

    void testTuningErrors_data()
    {
        QTest::addColumn<QByteArray>("notes");
        QTest::addColumn<QString>("expected"); // substring of the error

        QTest::newRow("invalid note") << json("['E2', 'Z9', 'D3']") << "Note 2 (\"Z9\")";
        QTest::newRow("unicode sharp") << json("['E2', 'A2', 'F♯3']") << "Note 3";
        QTest::newRow("out of range") << json("['E9', 'A2', 'D3']") << "Note 1";
        QTest::newRow("number instead of note") << json("[40, 'A2', 'D3']") << "Note 1 (40)";
        QTest::newRow("too few notes") << json("['E2', 'A2']") << "lists 2 notes";
        QTest::newRow("too many notes") << json("['E2', 'A2', 'D3', 'G3']") << "lists 4 notes";
        QTest::newRow("not an array") << json("'E2 A2 D3'") << "must be an array";
    }

    void testTuningErrors()
    {
        QFETCH(QByteArray, notes);
        QFETCH(QString, expected);

        const Tuning tuning = parseNotes(notes);
        QVERIFY(!tuning.isValid());
        QVERIFY(tuning.notes.isEmpty());
        QVERIFY2(tuning.error.contains(expected), qPrintable(tuning.error));
    }

    void testBrokenTuningKeepsInstrumentValid()
    {
        const Config config = parseInstrument(json("{'name': 'Bass', 'stringCount': 1, 'tunings': ["
            "{'name': 'Good', 'stringNotes': ['E1']},"
            "{'name': 'Bad', 'stringNotes': ['X1']},"
            "{'stringNotes': ['E1']}"
        "]}"));
        const Instrument &instrument = config.instruments.first();
        QVERIFY(instrument.isValid());
        QCOMPARE(instrument.tunings.size(), 3);
        QVERIFY(instrument.tunings.at(0).isValid());
        QVERIFY(!instrument.tunings.at(1).isValid());
        QCOMPARE(instrument.tunings.at(2).name, QStringLiteral("Tuning 3"));
        QVERIFY(!instrument.tunings.at(2).isValid());
    }

    void testBrokenInstrumentKeepsOthers()
    {
        const Config config = parse(json("{'instruments': ["
            "{'name': 'Good', 'stringCount': 1, 'tunings': [{'name': 'T', 'stringNotes': ['E1']}]},"
            "{'name': 'Bad', 'stringCount': -3, 'tunings': [{'name': 'T', 'stringNotes': ['E1']}]}"
        "]}"));
        QCOMPARE(config.instruments.size(), 2);
        QVERIFY(config.instruments.at(0).isValid());
        QVERIFY(!config.instruments.at(1).isValid());
    }

    void testDuplicateNames()
    {
        const Config config = parse(json("{'instruments': ["
            "{'name': 'Guitar', 'stringCount': 1, 'tunings': ["
                "{'name': 'Low', 'stringNotes': ['E2']},"
                "{'name': 'Low', 'stringNotes': ['D2']},"
                "{'name': 'High', 'stringNotes': ['E4']}"
            "]},"
            "{'name': 'Guitar', 'stringCount': 1, 'tunings': [{'name': 'T', 'stringNotes': ['E2']}]},"
            "{'name': 'Bass', 'stringCount': 1, 'tunings': [{'name': 'T', 'stringNotes': ['E1']}]}"
        "]}"));
        QCOMPARE(config.instruments.size(), 3);
        QVERIFY(config.instruments.at(0).error.contains(QStringLiteral("also named \"Guitar\"")));
        QVERIFY(!config.instruments.at(1).isValid());
        QVERIFY(config.instruments.at(2).isValid());

        const QList<Tuning> &tunings = config.instruments.at(0).tunings;
        QVERIFY(tunings.at(0).error.contains(QStringLiteral("also named \"Low\"")));
        QVERIFY(!tunings.at(1).isValid());
        QVERIFY(tunings.at(2).isValid());
    }
};

QTEST_GUILESS_MAIN(InstrumentConfigTest)
#include "instrumentconfigtest.moc"
