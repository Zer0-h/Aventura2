A l'hora d'imprimir el prompt si el directori al que esteim comença per /home/username ho substituirem per ~ tal i com fa bash.

S'ha comprobat que funcioni per a directoris aparentment conflictius, un cas que funciona com a bash és:
cd Aventura2/'prueba dir'/p\r\u\e\b\a/\"/'pr\"ueba dir larga'/pr\'\"\\ueba/prueba\ dir
que ens duu al directori /home/user/Aventura2/prueba dir/prueba/"/pr"ueba dir larga/pr'"\ueba/prueba dir

^^^^^ deixar guapo

Quan arribem al límit de jobs que podem tenir es shell ho comprobarà abans de llançar es fork si el procés és de background, si esteim al límit i llançam
un procés a foreground funcionarà normalment, però si llançam CTRL+Z notificarem que esteim al límit de jobs i no l'aturarem, seguirà en foreground.

El comando jobs ens permet ficar un argument (Sintaxis: jobs [JOB_SPEC]) ens donarà la informació sobre el treball que li hem demanat. Amb control d'errors
en el cas que no existesqui el treball.

