Autor: Antonio Torres Escribano

A l'hora de construir el prompt si el directori al que esteim comença per
/home/username ho substituirem per ~ tal i com fa bash.

internal_cd permet l'us de \', \" o amb qualsevol altre caràcter per indicar que
el directori conté el que be després de \ dins el seu nom.
D'aquí ve la seva complexitat.

El comando jobs i bg permet l'us d'un o més arguments per mostrar els que
especificam i per possar en background més d'un treball a la vegada
respectivament.

Quan arribem al límit de jobs que podem tenir es shell ho comprobarà abans de
llançar es fork si el procés és de background, si esteim al límit i llançam un
procés a foreground funcionarà normalment, però si llançam CTRL+Z notificarem
que esteim al límit de jobs i no l'aturarem, seguirà en foreground.
(s'ha comprobat amb un N_JOBS baix per facilitar el treball)

En lloc de tenir un condicional per SIGTSTP ho ignoram sempre, a pare i a fill,
en es controlador enviarem la senyal SIGSTOP. Abans creava un petit conflicte,
si el procés no era background feiem l'acció per defecte, és a dir, aturar el
programa. I en es controlador el tornavem a aturar, cosa que no te gaire sentit.
Ara nosaltres som els que decidim que s'aturi dins el controlador, no fora. 

Copiam command_line dins parse args en lloc de copiar directament la línia que
ens han entrat a execute line. Així si possam més espais de la conta ho deixam
igual que ho tenim a l'array d'strings.