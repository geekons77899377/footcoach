#pragma once

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QVector>

// GIF animé (GIF89a, boucle infinie), sans bibliothèque externe : palette fixe de 252 couleurs (6 rouges × 7 verts ×
// 6 bleus) plus la couleur transparente, compression LZW. Chaque image ne contient que le rectangle qui a changé
// depuis la précédente (le reste est transparent), ce qui garde le fichier léger pour un terrain animé.
class GifWriter
{
public:
    // delayCs : durée d'une image en centièmes de seconde.
    void begin(QSize size, int delayCs);
    // Image mise à l'échelle de la taille choisie (rapport conservé, bandes de la couleur du terrain sinon).
    void addFrame(const QImage &image);
    QByteArray finish();
    void clear();
    bool isActive() const { return m_active; }
    int frameCount() const { return m_frames; }
    QSize size() const { return m_size; }

    // Indice de palette d'une couleur (exposé pour les tests).
    static int paletteIndex(QRgb c);
    static QRgb paletteColor(int index);
    static constexpr int kTransparent = 255;

private:
    void writeFrame(const QVector<uchar> &indices, int x, int y, int w, int h);
    static QByteArray lzw(const QVector<uchar> &indices);

    QByteArray m_out;
    QVector<uchar> m_previous;
    QSize m_size;
    int m_delay = 10;
    int m_frames = 0;
    bool m_active = false;
};
