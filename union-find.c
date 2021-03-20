/*
 * TP1 | Info602
 * Kevin Traini
 * Jules Geyer
 */
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <time.h>

//-----------------------------------------------------------------------------
// Déclaration des types
//-----------------------------------------------------------------------------
/**
   Le contexte contient les informations utiles de l'interface pour
   les algorithmes de traitement d'image.
*/
typedef struct SContexte {
    int width;
    int height;
    GdkPixbuf *pixbuf_input;
    GdkPixbuf *pixbuf_output;
    GtkWidget *image;
    GtkWidget *seuil_widget;
} Contexte;

/**
   Un pixel est une structure de 3 octets (rouge, vert, bleu). On les
   plaque au bon endroit dans un pixbuf pour modifier les couleurs du pixel.
 */
typedef struct {
    guchar rouge;
    guchar vert;
    guchar bleu;
} Pixel;

/// Un Objet stocke donc un pointeur vers son pixel, son rang et un pointeur vers l'objet père.
typedef struct SObjet {
    Pixel *pixel; // adresse du pixel dans le pixbuf
    int rang;
    struct SObjet *pere;
} Objet;

typedef struct {
    double rouge;
    double vert;
    double bleu;
    int nb;
} StatCouleur;

//-----------------------------------------------------------------------------
// Déclaration des fonctions
//-----------------------------------------------------------------------------
gboolean selectInput(GtkWidget *widget, gpointer data);

gboolean selectOutput(GtkWidget *widget, gpointer data);

GtkWidget *creerIHM(const char *image_filename, Contexte *pCtxt);

void analyzePixbuf(GdkPixbuf *pixbuf);

GdkPixbuf *creerImage(int width, int height);

unsigned char greyLevel(Pixel *data);

void setGreyLevel(Pixel *data, unsigned char g);

Pixel *gotoPixel(GdkPixbuf *pixbuf, int x, int y);

void disk(GdkPixbuf *pixbuf, int r);

gboolean seuiller(GtkWidget *widget, gpointer data);

gboolean composantesConnexes(GtkWidget *widget, gpointer data);

gboolean composantesConnexesMoy(GtkWidget *widget, gpointer data);

int estRacine(Objet *obj);

Objet *CreerEnsembles(GdkPixbuf *pixbuf);

// Retourne le représentant de l'objet obj
Objet *TrouverEnsemble(Objet *obj);

// Si obj1 et obj2 n'ont pas les mêmes représentants, appelle Lier sur leurs représentants
void Union(Objet *obj1, Objet *obj2);

// Si obj1 et obj2 sont tous deux des racines, et sont distincts, alors réalise l'union des deux arbres.
void Lier(Objet *obj1, Objet *obj2);


//-----------------------------------------------------------------------------
// Programme principal
//-----------------------------------------------------------------------------
int main(int argc,
         char *argv[]) {
    // Initialisation de rand
    time_t t;
    srand((unsigned) time(&t));

    Contexte context;
    const char *image_filename = argc > 1 ? argv[1] : "lena.png";

    /* Passe les arguments à GTK, pour qu'il extrait ceux qui le concernent. */
    gtk_init(&argc, &argv);

    /* Crée une fenêtre. */
    creerIHM(image_filename, &context);

    /* Rentre dans la boucle d'événements. */
    gtk_main();
    return 0;
}


/// Fonction appelée lorsqu'on clique sur "Input".
gboolean selectInput(GtkWidget *widget, gpointer data) {
    // Récupère le contexte.
    Contexte *pCtxt = (Contexte *) data;
    // Place le pixbuf à visualiser dans le bon widget.
    gtk_image_set_from_pixbuf(GTK_IMAGE(pCtxt->image), pCtxt->pixbuf_input);
    // Force le réaffichage du widget.
    gtk_widget_queue_draw(pCtxt->image);
    return TRUE;
}

/// Fonction appelée lorsqu'on clique sur "Output".
gboolean selectOutput(GtkWidget *widget, gpointer data) {
    // Récupère le contexte.
    Contexte *pCtxt = (Contexte *) data;
    // Place le pixbuf à visualiser dans le bon widget.
    gtk_image_set_from_pixbuf(GTK_IMAGE(pCtxt->image), pCtxt->pixbuf_output);
    // Force le réaffichage du widget.
    gtk_widget_queue_draw(pCtxt->image);
    return TRUE;
}

/// Fonction appelée lorsque l'on clique sur seuiller
gboolean seuiller(GtkWidget *widget, gpointer data) {
    // Récupérer le contexte.
    Contexte *pCtxt = (Contexte *) data;
    guchar *dIn = gdk_pixbuf_get_pixels(pCtxt->pixbuf_input);
    guchar *dOut = gdk_pixbuf_get_pixels(pCtxt->pixbuf_output);
    int rowstride = gdk_pixbuf_get_rowstride(pCtxt->pixbuf_input);
    int t = gtk_range_get_value((GtkRange *) pCtxt->seuil_widget);

    for (int y = 0; y < pCtxt->height; ++y) {
        Pixel *pixel = (Pixel *) dIn;
        Pixel *pixel2 = (Pixel *) dOut;
        for (int x = 0; x < pCtxt->width; ++x) {
            if (greyLevel(pixel) < t)
                setGreyLevel(pixel2, 0);
            else
                setGreyLevel(pixel2, 255);
            ++pixel;
            ++pixel2;
        }
        dIn += rowstride; // passe à la ligne suivante
        dOut += rowstride; // passe à la ligne suivante
    }

    // Place le pixbuf à visualiser dans le bon widget.
    gtk_image_set_from_pixbuf(GTK_IMAGE(pCtxt->image), pCtxt->pixbuf_output);
    // Force le réaffichage du widget.
    gtk_widget_queue_draw(pCtxt->image);

    return TRUE;
}

gboolean composantesConnexes(GtkWidget *widget, gpointer data) {
    struct timespec myTimerStart;
    clock_gettime(CLOCK_REALTIME, &myTimerStart);

    // Récupérer le contexte.
    Contexte *pCtxt = (Contexte *) data;
    int width = pCtxt->width;
    int height = pCtxt->height;
    Objet *objets = CreerEnsembles(pCtxt->pixbuf_output);

    // Parcours horizontal
    for (int i = 0; i < (width * height) - 1; i++) {
        if (greyLevel(objets[i].pixel) == greyLevel(objets[i + 1].pixel))
            Union(&objets[i], &objets[i + 1]);
    }

    // Parcours vertical
    for (int i = 0; i < (width * (height - 1)) - 1; i++) {
        if (greyLevel(objets[i].pixel) == greyLevel(objets[i + width].pixel))
            Union(&objets[i], &objets[i + width]);
    }

    for (int i = 0; i < (width * height); i++) {
        if (estRacine(&objets[i])) {
            objets[i].pixel->rouge = rand() % 256;
            objets[i].pixel->vert = rand() % 256;
            objets[i].pixel->bleu = rand() % 256;
        }
    }

    for (int i = 0; i < (width * height); i++) {
        objets[i].pixel->rouge = TrouverEnsemble(&objets[i])->pixel->rouge;
        objets[i].pixel->vert = TrouverEnsemble(&objets[i])->pixel->vert;
        objets[i].pixel->bleu = TrouverEnsemble(&objets[i])->pixel->bleu;
    }

    // Place le pixbuf à visualiser dans le bon widget.
    gtk_image_set_from_pixbuf(GTK_IMAGE(pCtxt->image), pCtxt->pixbuf_output);
    // Force le réaffichage du widget.
    gtk_widget_queue_draw(pCtxt->image);

    struct timespec current;
    clock_gettime(CLOCK_REALTIME, &current);
    double t = ((current.tv_sec - myTimerStart.tv_sec) * 1000 + (current.tv_nsec - myTimerStart.tv_nsec) / 1000000.0);
    printf("time = %lf ms.\n", t);

    return TRUE;
}

/**
 * Tableau des temps
 * * --------------------------------------------------------------------- *
 * | Image            | Taille   | Temps 1   | Temps 2 | Temps 3 | Temps 4 |
 * * --------------------------------------------------------------------- *
 * | papillon-express | 605x418  | 26737ms   | 37ms    | 29ms    | 64ms    |
 * | edt              | 557x591  | 35061ms   | 39ms    | 37ms    | 37ms    |
 * | lena             | 256x256  | 1082ms    | 33ms    | 7ms     | 11ms    |
 * | kowloon-1000     | 1000x655 | 42129ms   | 122ms   | 90ms    | 107ms   |
 * * --------------------------------------------------------------------- *
 * Entre l'algo 'bête' et la première heuristique, le temps est déjà divisé par 100
 * Puis en rajoutant le deuxième heuristique on trouve un temps quasi linéaire par rapport à la taille de l'image
 */

gboolean composantesConnexesMoy(GtkWidget *widget, gpointer data) {
    struct timespec myTimerStart;
    clock_gettime(CLOCK_REALTIME, &myTimerStart);

    // Récupérer le contexte.
    Contexte *pCtxt = (Contexte *) data;
    int width = pCtxt->width;
    int height = pCtxt->height;
    Objet *objets = CreerEnsembles(pCtxt->pixbuf_output);

    // Parcours horizontal
    for (int i = 0; i < (width * height) - 1; i++) {
        if (greyLevel(objets[i].pixel) == greyLevel(objets[i + 1].pixel))
            Union(&objets[i], &objets[i + 1]);
    }

    // Parcours vertical
    for (int i = 0; i < (width * (height - 1)) - 1; i++) {
        if (greyLevel(objets[i].pixel) == greyLevel(objets[i + width].pixel))
            Union(&objets[i], &objets[i + width]);
    }

    int size = width * height;
    StatCouleur *stats = (StatCouleur *) malloc(size * sizeof(StatCouleur));

    for (int i = 0; i < (width * height); i++) {
        if (estRacine(&objets[i])) {
            stats[i].rouge = 0;
            stats[i].vert = 0;
            stats[i].bleu = 0;
        }
    }

    guchar *data_input = gdk_pixbuf_get_pixels(pCtxt->pixbuf_input);
    guchar *data_output = gdk_pixbuf_get_pixels(pCtxt->pixbuf_output);
    for (int i = 0; i < size; i++) {
        Objet *rep = TrouverEnsemble(&objets[i]);
        long int j = (rep - objets);
        Pixel *pixel_src = (Pixel *) (data_input + ((guchar *) objets[i].pixel - data_output));
        // pixel_src est la couleur de ce pixel dans l'image input.
        // On l'ajoute à la stat du représentant j.
        if (j >= 0 && j < size) {
            stats[j].rouge += pixel_src->rouge;
            stats[j].vert += pixel_src->vert;
            stats[j].bleu += pixel_src->bleu;
            stats[j].nb += 1; // On aura donc la somme cumulée
        }
    }

    for (int i = 0; i < size; i++) {
        if (estRacine(&objets[i])) {
            objets[i].pixel->rouge = stats[i].rouge / stats[i].nb;
            objets[i].pixel->vert = stats[i].vert / stats[i].nb;
            objets[i].pixel->bleu = stats[i].bleu / stats[i].nb;
        }
    }

    for (int i = 0; i < (width * height); i++) {
        objets[i].pixel->rouge = TrouverEnsemble(&objets[i])->pixel->rouge;
        objets[i].pixel->vert = TrouverEnsemble(&objets[i])->pixel->vert;
        objets[i].pixel->bleu = TrouverEnsemble(&objets[i])->pixel->bleu;
    }

    // Place le pixbuf à visualiser dans le bon widget.
    gtk_image_set_from_pixbuf(GTK_IMAGE(pCtxt->image), pCtxt->pixbuf_output);
    // Force le réaffichage du widget.
    gtk_widget_queue_draw(pCtxt->image);

    struct timespec current;
    clock_gettime(CLOCK_REALTIME, &current);
    double t = ((current.tv_sec - myTimerStart.tv_sec) * 1000 + (current.tv_nsec - myTimerStart.tv_nsec) / 1000000.0);
    printf("time = %lf ms.\n", t);

    return TRUE;
}

/// Charge l'image donnée et crée l'interface.
GtkWidget *creerIHM(const char *image_filename, Contexte *pCtxt) {
    GtkWidget *window;
    GtkWidget *vbox1;
    GtkWidget *vbox2;
    GtkWidget *hbox1;
    GtkWidget *button_seuiller;
    GtkWidget *button_composantes;
    GtkWidget *button_composantes_moy;
    GtkWidget *button_quit;
    GtkWidget *button_select_input;
    GtkWidget *button_select_output;
    GError **error = NULL;

    /* Crée une fenêtre. */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    // Crée un conteneur horitzontal box.
    hbox1 = gtk_hbox_new(FALSE, 10);
    // Crée deux conteneurs vertical box.
    vbox1 = gtk_vbox_new(FALSE, 10);
    vbox2 = gtk_vbox_new(FALSE, 10);
    // Crée le pixbuf source et le pixbuf destination
    pCtxt->pixbuf_input = gdk_pixbuf_new_from_file(image_filename, error);
    pCtxt->width = gdk_pixbuf_get_width(pCtxt->pixbuf_input);           // Largeur de l'image en pixels
    pCtxt->height = gdk_pixbuf_get_height(pCtxt->pixbuf_input);          // Hauteur de l'image en pixels
    pCtxt->pixbuf_output = creerImage(pCtxt->width, pCtxt->height);
    analyzePixbuf(pCtxt->pixbuf_input);
    disk(pCtxt->pixbuf_output, 100);
    // Crée le widget qui affiche le pixbuf image.
    pCtxt->image = gtk_image_new_from_pixbuf(pCtxt->pixbuf_input);
    // Rajoute l'image dans le conteneur hbox.
    gtk_container_add(GTK_CONTAINER(hbox1), pCtxt->image);
    // Rajoute le 2eme vbox dans le conteneur hbox (pour mettre les boutons sélecteur d'image).
    gtk_container_add(GTK_CONTAINER(hbox1), vbox2);
    // Crée les boutons de sélection "source"/"destination".
    button_select_input = gtk_button_new_with_label("Input");
    button_select_output = gtk_button_new_with_label("Output");
    // Connecte la réaction gtk_main_quit à l'événement "clic" sur ce bouton.
    g_signal_connect(button_select_input, "clicked",
                     G_CALLBACK(selectInput),
                     pCtxt);
    g_signal_connect(button_select_output, "clicked",
                     G_CALLBACK(selectOutput),
                     pCtxt);
    gtk_container_add(GTK_CONTAINER(vbox2), button_select_input);
    gtk_container_add(GTK_CONTAINER(vbox2), button_select_output);
    // Crée le bouton quitter.
    button_quit = gtk_button_new_with_label("Quitter");
    // Connecte la réaction gtk_main_quit à l'événement "clic" sur ce bouton.
    g_signal_connect(button_quit, "clicked",
                     G_CALLBACK(gtk_main_quit),
                     NULL);
    // Crée le gtk scale
    pCtxt->seuil_widget = gtk_hscale_new_with_range(0, 255, 1);
    // Crée le bouton seuiller
    button_seuiller = gtk_button_new_with_label("Seuiller");
    // Connecte la réaction seuiller à l'événement "clic" sur ce bouton.
    g_signal_connect(button_seuiller, "clicked",
                     G_CALLBACK(seuiller),
                     pCtxt);
    // Crée le bouton composantes
    button_composantes = gtk_button_new_with_label("Composantes connexes");
    // Connecte la réaction ComposantesConnexes à l'événement "clic" sur ce bouton.
    g_signal_connect(button_composantes, "clicked",
                     G_CALLBACK(composantesConnexes),
                     pCtxt);
    // Crée le bouton composantes moyennes
    button_composantes_moy = gtk_button_new_with_label("Composantes connexes (Couleur moyenne)");
    // Connecte la réaction ComposantesConnexes à l'événement "clic" sur ce bouton.
    g_signal_connect(button_composantes_moy, "clicked",
                     G_CALLBACK(composantesConnexesMoy),
                     pCtxt);
    // Rajoute tout dans le conteneur vbox.
    gtk_container_add(GTK_CONTAINER(vbox1), hbox1);
    gtk_container_add(GTK_CONTAINER(vbox1), pCtxt->seuil_widget);
    gtk_container_add(GTK_CONTAINER(vbox1), button_seuiller);
    gtk_container_add(GTK_CONTAINER(vbox1), button_composantes);
    gtk_container_add(GTK_CONTAINER(vbox1), button_composantes_moy);
    gtk_container_add(GTK_CONTAINER(vbox1), button_quit);
    // Rajoute la vbox  dans le conteneur window.
    gtk_container_add(GTK_CONTAINER(window), vbox1);

    // Rend tout visible
    gtk_widget_show_all(window);

    return window;
}

/**
    Utile pour vérifier que le GdkPixbuf a un formal usuel: 3 canaux RGB, 24 bits par pixel,
    et que la machine supporte l'alignement de la structure sur 3 octets.
*/
void analyzePixbuf(GdkPixbuf *pixbuf) {
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);      // Nb de canaux (Rouge, Vert, Bleu, potentiellement Alpha)
    int has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);       // Dit s'il y a un canal Alpha (transparence).
    int bits_per_sample = gdk_pixbuf_get_bits_per_sample(
            pixbuf); // Donne le nombre de bits par échantillon (8 bits souvent).
    guchar *data = gdk_pixbuf_get_pixels(pixbuf);          // Pointeur vers le tampon de données
    int width = gdk_pixbuf_get_width(pixbuf);           // Largeur de l'image en pixels
    int height = gdk_pixbuf_get_height(pixbuf);          // Hauteur de l'image en pixels
    int rowstride = gdk_pixbuf_get_rowstride(
            pixbuf);       // Nombre d'octets entre chaque ligne dans le tampon de données
    printf("n_channels = %d\n", n_channels);
    printf("has_alpha  = %d\n", has_alpha);
    printf("bits_per_sa= %d\n", bits_per_sample);
    printf("width      = %d\n", width);
    printf("height     = %d\n", height);
    printf("data       = %p\n", data);
    printf("rowstride  = %d\n", rowstride);
    Pixel *pixel = (Pixel *) data;
    printf("sizeof(Pixel)=%ld\n", sizeof(Pixel));
    size_t diff = ((guchar * )(pixel + 1)) - (guchar *) pixel;
    printf("(pixel+1) - pixel=%ld\n", diff);
    assert(n_channels == 3);
    assert(has_alpha == FALSE);
    assert(bits_per_sample == 8);
    assert(sizeof(Pixel) == 3);
    assert(diff == 3);
}

/**
   Crée un image vide de taille width x height.
*/
GdkPixbuf *creerImage(int width, int height) {
    GdkPixbuf *img = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 0, 8, width, height);
    return img;
}

/**
   Retourne le niveau de gris du pixel.
*/
unsigned char greyLevel(Pixel *data) {
    return (data->rouge + data->vert + data->bleu) / 3;
}

/**
   Met le pixel au niveau de gris \a g.
*/
void setGreyLevel(Pixel *data, unsigned char g) {
    data->rouge = g;
    data->vert = g;
    data->bleu = g;
}

/**
    Va au pixel de coordonnées (x,y) dans le pixbuf.
*/
Pixel *gotoPixel(GdkPixbuf *pixbuf, int x, int y) {
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *data = gdk_pixbuf_get_pixels(pixbuf);
    return (Pixel *) (data + y * rowstride + x * 3);
}

/**
   Crée une image sous forme de disque.
*/
void disk(GdkPixbuf *pixbuf, int r) {
    int x, y;
    int width = gdk_pixbuf_get_width(pixbuf);           // Largeur de l'image en pixels
    int height = gdk_pixbuf_get_height(pixbuf);          // Hauteur de l'image en pixels
    int x0 = width / 2;
    int y0 = height / 2;
    for (y = 0; y < height; ++y) {
        Pixel *pixel = gotoPixel(pixbuf, 0, y); // les lignes peuvent être réalignées
        for (x = 0; x < width; ++x) {
            int d2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
            if (d2 >= r * r) setGreyLevel(pixel, 0);
            else setGreyLevel(pixel, 255 - (int) sqrt(d2));
            ++pixel; // sur une ligne, les pixels se suivent
        }
    }
}

Objet *CreerEnsembles(GdkPixbuf *pixbuf) {
    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    Objet *res = malloc(width * height * sizeof(Objet));

    guchar *data = gdk_pixbuf_get_pixels(pixbuf); // Pointeur vers le tampon de données
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf); // Nombre d'octets entre chaque ligne dans le tampon de données

    int i = 0;
    for (int y = 0; y < height; y++) {
        Pixel *pixel = (Pixel *) data;
        for (int x = 0; x < width; ++x) {
            res[i].pere = &res[i];
            res[i].rang = 0;
            res[i].pixel = pixel;

            i++;
            pixel++;
        }
        data += rowstride; // passe à la ligne suivante
    }

    return res;
}

int estRacine(Objet *obj) {
    return obj->pere == obj;
}

// Retourne le représentant de l'objet obj
Objet *TrouverEnsemble(Objet *obj) {
    if (!estRacine(obj))
        obj->pere = TrouverEnsemble(obj->pere);

    return obj->pere;
}

// Si obj1 et obj2 n'ont pas les mêmes représentants, appelle Lier sur leurs représentants
void Union(Objet *obj1, Objet *obj2) {
    Objet *racine1 = TrouverEnsemble(obj1);
    Objet *racine2 = TrouverEnsemble(obj2);
    if (racine1 != racine2) {
        if (racine1->rang < racine2->rang)
            Lier(racine1, racine2);
        else {
            Lier(racine2, racine1);
            if (racine1->rang == racine2->rang) {
                racine1->rang++;
            }
        }
    }
}

// Si obj1 et obj2 sont tous deux des racines, et sont distincts, alors réalise l'union des deux arbres.
void Lier(Objet *obj1, Objet *obj2) {
    if (estRacine(obj1) && estRacine(obj2))
        if (obj1 != obj2)
            obj1->pere = obj2;
}
