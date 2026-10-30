#pragma once

#include <QImage>
#include <QMap>
#include <QString>

namespace PdfExport {

// Écrit le HTML (sous-ensemble Qt) dans un PDF A4, marges de 15 mm. Renvoie false et remplit *error en cas d'échec.
// `images` : images référencées par <img src="nom"> dans le HTML.
bool writeHtmlToPdf(const QString &filePath, const QString &html, QString *error = nullptr,
                    const QMap<QString, QImage> &images = {});

} // namespace PdfExport
