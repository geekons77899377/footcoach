const fs=require('fs');eval(fs.readFileSync(require('path').join(__dirname,'..','..','qml','Keyboard.js'),'utf8').replace('.pragma library',''));
const assert=require('assert');
// clavier fermé -> 0
assert.equal(height({y:500,height:300},800,2.6,false),0);
// rectangle en pixels logiques cohérent -> inchangé
assert.equal(height({y:520,height:280},800,2.6,true),280);
// rectangle en pixels de l'appareil (déborde de la fenêtre) -> ramené par le rapport de pixels
assert.ok(Math.abs(height({y:1352,height:728},800,2.6,true)-280)<0.01);
// rectangle vide mais clavier ouvert -> repli 42 %
assert.equal(height({y:0,height:0},800,2.6,true),Math.round(800*0.42));
assert.equal(height(null,800,1,true),Math.round(800*0.42));
// valeur aberrante plafonnée à 70 %
assert.equal(height({y:100,height:790},800,1,true),560);
// rapport de pixels 1 : pas de conversion
assert.equal(height({y:520,height:280},800,1,true),280);
console.log('Keyboard.js : tous les tests passent');
