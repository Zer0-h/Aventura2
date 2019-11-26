A l'hora d'imprimir el prompt si el directori al que esteim comença per /home/username ho substituirem per ~ tal i com fa bash.

S'ha comprobat que funcioni per a directoris aparentment conflictius, un cas que funciona com a bash és:
cd Aventura2/'prueba dir'/p\r\u\e\b\a/\"/'pr\"ueba dir larga'/pr\'\"\\ueba/prueba\ dir
que ens duu al directori /home/user/Aventura2/prueba dir/prueba/"/pr"ueba dir larga/pr'"\ueba/prueba dir

^^^^^ deixar guapo

El comando jobs permet l'us d'un o més arguments per mostrar els que volguem, no tots com fa per defecte.

Quan arribem al límit de jobs que podem tenir es shell ho comprobarà abans de llançar es fork si el procés és de background, si esteim al límit i llançam
un procés a foreground funcionarà normalment, però si llançam CTRL+Z notificarem que esteim al límit de jobs i no l'aturarem, seguirà en foreground.


Copiam command_line dins parse args en lloc de copiar directament la línia que ens han entrar a execute line. Així si possam més espais de la conta,
no és un problema per executar-ho però se queda guardat amb l'extra d'espais