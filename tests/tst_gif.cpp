// GIF animé de l'animation d'un schéma (16/11/2026) : relu par le lecteur GIF de Qt (images, couleurs, boucle).
#include "app/Backend.h"
#include "app/GifWriter.h"
#include "core/Database.h"

#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest>

class GifTest : public QObject
{
    Q_OBJECT

    static QImage frame(QSize size, QPoint ball)
    {
        QImage img(size, QImage::Format_RGB32);
        img.fill(qRgb(0x2e, 0x7d, 0x32));
        QPainter p(&img);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(ball, 8, 8);
        p.setBrush(QColor("#1565c0"));
        p.drawRect(10, 10, 30, 20);
        return img;
    }

    static int dist(QRgb a, QRgb b)
    {
        return std::abs(qRed(a) - qRed(b)) + std::abs(qGreen(a) - qGreen(b)) + std::abs(qBlue(a) - qBlue(b));
    }

private slots:
    void paletteIsCloseToTheColours()
    {
        for (QRgb c : {qRgb(0, 0, 0), qRgb(255, 255, 255), qRgb(0x2e, 0x7d, 0x32), qRgb(0x15, 0x65, 0xc0), qRgb(0xc6, 0x28, 0x28)}) {
            const int i = GifWriter::paletteIndex(c);
            QVERIFY(i >= 0 && i < 252);
            QVERIFY2(dist(GifWriter::paletteColor(i), c) <= 90, qPrintable(QString::number(i)));
        }
    }

    void animatedGifIsReadBackFrameByFrame()
    {
        GifWriter w;
        const QSize size(200, 120);
        w.begin(size, 10);
        QList<QPoint> balls;
        for (int k = 0; k < 12; ++k)
            balls << QPoint(20 + k * 14, 60 + (k % 3) * 10);
        for (const QPoint &b : balls)
            w.addFrame(frame(size, b));
        w.addFrame(frame(size, balls.last()));                 // image identique : un pixel transparent
        QCOMPARE(w.frameCount(), 13);
        QByteArray gif = w.finish();
        QVERIFY(gif.startsWith("GIF89a"));
        QVERIFY(gif.endsWith(char(0x3B)));
        QVERIFY(gif.contains("NETSCAPE2.0"));
        QVERIFY(!w.isActive());

        QBuffer buf(&gif);
        buf.open(QIODevice::ReadOnly);
        QImageReader reader(&buf, "gif");
        QVERIFY2(reader.canRead(), qPrintable(reader.errorString()));
        QCOMPARE(reader.imageCount(), 13);
        QCOMPARE(reader.loopCount(), -1);                       // boucle infinie
        for (int k = 0; k < 13; ++k) {
            const QImage img = reader.read();
            QVERIFY2(!img.isNull(), qPrintable(reader.errorString()));
            QCOMPARE(img.size(), size);
            QCOMPARE(reader.nextImageDelay(), k < 12 ? 100 : 100);
            const QPoint b = balls.at(std::min(k, 11));
            QVERIFY2(dist(img.pixel(b), qRgb(255, 255, 255)) < 40, qPrintable(QStringLiteral("ballon, image %1").arg(k)));
            QVERIFY(dist(img.pixel(25, 20), qRgb(0x15, 0x65, 0xc0)) < 90);                 // joueur, toujours là
            QVERIFY(dist(img.pixel(190, 10), qRgb(0x2e, 0x7d, 0x32)) < 90);                // pelouse
            if (k > 0 && k < 12) {
                // L'ancienne position du ballon est redevenue pelouse.
                QVERIFY(dist(img.pixel(balls.at(k - 1)), qRgb(0x2e, 0x7d, 0x32)) < 90);
            }
        }
    }

    // Grande image peu compressible : la table LZW se remplit et repart de zéro sans erreur de lecture.
    void lzwTableResetsOnNoisyImages()
    {
        GifWriter w;
        const QSize size(300, 300);
        w.begin(size, 5);
        QImage img(size, QImage::Format_RGB32);
        quint32 seed = 12345;
        for (int y = 0; y < size.height(); ++y)
            for (int x = 0; x < size.width(); ++x) {
                seed = seed * 1103515245u + 12345u;
                img.setPixel(x, y, qRgb((seed >> 16) & 0xff, (seed >> 8) & 0xff, seed & 0xff));
            }
        w.addFrame(img);
        QByteArray gif = w.finish();
        QBuffer buf(&gif);
        buf.open(QIODevice::ReadOnly);
        QImageReader reader(&buf, "gif");
        const QImage back = reader.read();
        QVERIFY2(!back.isNull(), qPrintable(reader.errorString()));
        int ok = 0;
        for (int y = 0; y < size.height(); y += 7)
            for (int x = 0; x < size.width(); x += 7)
                ok += back.pixel(x, y) == GifWriter::paletteColor(GifWriter::paletteIndex(img.pixel(x, y)));
        QCOMPARE(ok, ((size.height() + 6) / 7) * ((size.width() + 6) / 7));
    }

    void backendWritesTheFile()
    {
        Database db;
        QString error;
        QVERIFY2(db.open(QStringLiteral(":memory:"), &error), qPrintable(error));
        Backend b(db.handle());
        QCOMPARE(b.animationFileName(QStringLiteral("Le parcours des portes")), QStringLiteral("animation-le-parcours-des-portes.gif"));
        QVERIFY(!b.gifFinish(QStringLiteral("file:///tmp/x.gif")).value("ok").toBool());   // rien commencé
        QVERIFY(!b.gifBegin(4, 4, 10));                                                     // trop petit
        QVERIFY(b.gifBegin(160, 100, 10));
        QVERIFY(b.gifAddFrame(frame(QSize(320, 200), QPoint(40, 50))));
        QVERIFY(b.gifAddFrame(frame(QSize(320, 200), QPoint(80, 50))));
        QVERIFY(!b.gifAddFrame(QImage()));
        QTemporaryDir dir;
        const QString out = dir.filePath(QStringLiteral("a.gif"));
        const QVariantMap r = b.gifFinish(QUrl::fromLocalFile(out).toString());
        QVERIFY2(r.value("ok").toBool(), qPrintable(r.value("error").toString()));
        QCOMPARE(r.value("frames").toInt(), 2);
        QImageReader reader(out);
        QCOMPARE(reader.imageCount(), 2);
        QCOMPARE(reader.size(), QSize(160, 100));
        // Annuler vide tout.
        QVERIFY(b.gifBegin(160, 100, 10));
        b.gifCancel();
        QVERIFY(!b.gifFinish(QUrl::fromLocalFile(out).toString()).value("ok").toBool());
        db.close();
    }
};

QTEST_MAIN(GifTest)
#include "tst_gif.moc"
