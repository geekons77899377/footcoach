.pragma library

// Hauteur du clavier à l'écran, en pixels de l'interface, à partir de Qt.inputMethod.keyboardRectangle.
// Sur Android, Qt a rendu ce rectangle tantôt en pixels de l'appareil, tantôt en pixels logiques selon les versions
// et les téléphones : si le rectangle déborde de la fenêtre, on le ramène par le rapport de pixels de l'écran.
// Si le clavier est ouvert mais que son rectangle est vide (clavier flottant ou téléphone qui ne le renseigne pas),
// on prend une hauteur de repli plutôt que de laisser le clavier cacher les champs.
//   rect : {y, height} ; winHeight : hauteur de la fenêtre ; dpr : devicePixelRatio ; visible : clavier ouvert
function height(rect, winHeight, dpr, visible) {
    if (!visible)
        return 0;
    var h = rect ? rect.height : 0;
    var y = rect ? rect.y : 0;
    if (dpr > 1 && h > 0 && y + h > winHeight * 1.05)
        h = h / dpr;                       // rectangle en pixels de l'appareil
    if (!(h > 0))
        return Math.round(winHeight * 0.42);  // repli : environ 42 % de la fenêtre
    return Math.min(h, winHeight * 0.7);   // jamais plus de 70 % de la fenêtre
}
