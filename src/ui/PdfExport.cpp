#include "PdfExport.h"

#include <QFile>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>
#include <QUrl>

namespace PdfExport {

bool writeHtmlToPdf(const QString &filePath, const QString &html, QString *error, const QMap<QString, QImage> &images)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = QStringLiteral("Impossible d'écrire le fichier : %1").arg(file.errorString());
        return false;
    }

    {
        QPdfWriter writer(&file);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
        writer.setTitle(QStringLiteral("Séance d'entraînement"));

        QTextDocument doc;
        for (auto it = images.constBegin(); it != images.constEnd(); ++it)
            doc.addResource(QTextDocument::ImageResource, QUrl(it.key()), it.value());
        doc.setHtml(html);
        doc.print(&writer);
    }   // le QPdfWriter termine le document à sa destruction

    file.close();
    if (file.size() <= 0) {
        if (error)
            *error = QStringLiteral("Le PDF généré est vide.");
        return false;
    }
    return true;
}

} // namespace PdfExport
