#include "GifWriter.h"

#include <QPainter>

#include <algorithm>
#include <unordered_map>

namespace {

void put16(QByteArray &out, int v)
{
    out.append(char(v & 0xff));
    out.append(char((v >> 8) & 0xff));
}

// Écrit des codes de longueur variable, bit de poids faible en premier, en blocs de 255 octets au plus.
class BitWriter
{
public:
    void write(int code, int bits)
    {
        m_acc |= quint32(code) << m_count;
        m_count += bits;
        while (m_count >= 8) {
            m_bytes.append(char(m_acc & 0xff));
            m_acc >>= 8;
            m_count -= 8;
        }
    }
    QByteArray finish()
    {
        if (m_count > 0)
            m_bytes.append(char(m_acc & 0xff));
        QByteArray out;
        for (int i = 0; i < m_bytes.size(); i += 255) {
            const int n = std::min(255, int(m_bytes.size()) - i);
            out.append(char(n));
            out.append(m_bytes.mid(i, n));
        }
        out.append(char(0));
        return out;
    }

private:
    QByteArray m_bytes;
    quint32 m_acc = 0;
    int m_count = 0;
};

} // namespace

int GifWriter::paletteIndex(QRgb c)
{
    const int r = (qRed(c) * 5 + 127) / 255;
    const int g = (qGreen(c) * 6 + 127) / 255;
    const int b = (qBlue(c) * 5 + 127) / 255;
    return r * 42 + g * 6 + b;                     // 0 à 251
}

QRgb GifWriter::paletteColor(int index)
{
    if (index >= 252)
        return qRgb(0, 0, 0);
    const int r = index / 42, g = (index / 6) % 7, b = index % 6;
    return qRgb(r * 255 / 5, g * 255 / 6, b * 255 / 5);
}

void GifWriter::clear()
{
    m_out.clear();
    m_previous.clear();
    m_frames = 0;
    m_active = false;
}

void GifWriter::begin(QSize size, int delayCs)
{
    clear();
    m_size = size.expandedTo(QSize(1, 1)).boundedTo(QSize(2000, 2000));
    m_delay = std::clamp(delayCs, 2, 1000);
    m_active = true;
    m_out.append("GIF89a");
    put16(m_out, m_size.width());
    put16(m_out, m_size.height());
    m_out.append(char(0xF7));                      // palette globale de 256 couleurs
    m_out.append(char(0));                         // fond : indice 0
    m_out.append(char(0));
    for (int i = 0; i < 256; ++i) {
        const QRgb c = paletteColor(i);
        m_out.append(char(qRed(c)));
        m_out.append(char(qGreen(c)));
        m_out.append(char(qBlue(c)));
    }
    // Boucle infinie (extension NETSCAPE2.0).
    m_out.append("\x21\xFF\x0B" "NETSCAPE2.0" "\x03\x01", 16);
    put16(m_out, 0);
    m_out.append(char(0));
}

void GifWriter::addFrame(const QImage &image)
{
    if (!m_active)
        return;
    const int W = m_size.width(), H = m_size.height();
    QImage frame(m_size, QImage::Format_RGB32);
    frame.fill(qRgb(0x24, 0x5a, 0x2b));            // fond du terrain autour
    if (!image.isNull()) {
        const QImage scaled = image.scaled(m_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPainter p(&frame);
        p.drawImage((W - scaled.width()) / 2, (H - scaled.height()) / 2, scaled);
    }
    QVector<uchar> idx(W * H);
    for (int y = 0; y < H; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(frame.constScanLine(y));
        for (int x = 0; x < W; ++x)
            idx[y * W + x] = uchar(paletteIndex(line[x]));
    }
    if (m_previous.isEmpty()) {
        writeFrame(idx, 0, 0, W, H);
    } else {
        // Rectangle qui a changé ; ailleurs, transparent.
        int x0 = W, y0 = H, x1 = -1, y1 = -1;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (idx[y * W + x] != m_previous[y * W + x]) {
                    x0 = std::min(x0, x);
                    x1 = std::max(x1, x);
                    y0 = std::min(y0, y);
                    y1 = std::max(y1, y);
                }
            }
        }
        if (x1 < 0) {                              // rien n'a changé : image d'un pixel transparent
            writeFrame(QVector<uchar>{uchar(kTransparent)}, 0, 0, 1, 1);
        } else {
            const int w = x1 - x0 + 1, h = y1 - y0 + 1;
            QVector<uchar> part(w * h);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    const int k = (y0 + y) * W + x0 + x;
                    part[y * w + x] = idx[k] == m_previous[k] ? uchar(kTransparent) : idx[k];
                }
            }
            writeFrame(part, x0, y0, w, h);
        }
    }
    m_previous = idx;
    ++m_frames;
}

void GifWriter::writeFrame(const QVector<uchar> &indices, int x, int y, int w, int h)
{
    // Contrôle graphique : durée, disposition 1 (garder l'image précédente), couleur transparente.
    m_out.append("\x21\xF9\x04", 3);
    m_out.append(char(0x05));
    put16(m_out, m_delay);
    m_out.append(char(kTransparent));
    m_out.append(char(0));
    // Descripteur d'image (palette globale, pas d'entrelacement).
    m_out.append(char(0x2C));
    put16(m_out, x);
    put16(m_out, y);
    put16(m_out, w);
    put16(m_out, h);
    m_out.append(char(0));
    m_out.append(char(8));                         // taille minimale des codes LZW
    m_out.append(lzw(indices));
}

QByteArray GifWriter::lzw(const QVector<uchar> &indices)
{
    const int clear = 256, eoi = 257;
    BitWriter bits;
    int codeSize = 9;
    int next = 258;
    std::unordered_map<int, int> dict;
    dict.reserve(8192);
    bits.write(clear, codeSize);
    if (indices.isEmpty()) {
        bits.write(eoi, codeSize);
        return bits.finish();
    }
    int prefix = indices[0];
    for (int i = 1; i < indices.size(); ++i) {
        const int c = indices[i];
        const int key = (prefix << 8) | c;
        const auto it = dict.find(key);
        if (it != dict.end()) {
            prefix = it->second;
            continue;
        }
        bits.write(prefix, codeSize);
        // Même règle que les décodeurs : une entrée de plus par code ; à 4096 entrées, on repart d'une table vide.
        const int assigned = next++;
        dict.emplace(key, assigned);
        if (assigned >= (1 << codeSize) && codeSize < 12)
            ++codeSize;
        if (assigned == 4095) {
            bits.write(clear, codeSize);
            dict.clear();
            next = 258;
            codeSize = 9;
        }
        prefix = c;
    }
    bits.write(prefix, codeSize);
    bits.write(eoi, codeSize);
    return bits.finish();
}

QByteArray GifWriter::finish()
{
    if (!m_active)
        return QByteArray();
    m_out.append(char(0x3B));
    QByteArray out = m_out;
    clear();
    return out;
}
