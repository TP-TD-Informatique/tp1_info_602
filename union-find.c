#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <stdio.h>

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


//-----------------------------------------------------------------------------
// Programme principal
//-----------------------------------------------------------------------------
int main(int argc,
         char *argv[]) {
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

/// Charge l'image donnée et crée l'interface.
GtkWidget *creerIHM(const char *image_filename, Contexte *pCtxt) {
    GtkWidget *window;
    GtkWidget *vbox1;
    GtkWidget *vbox2;
    GtkWidget *hbox1;
    GtkWidget *button_seuiller;
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
    //Connecte la réaction gtk_main_seuiller à l'événement "clic" sur ce bouton.
    g_signal_connect(button_seuiller, "clicked",
                     G_CALLBACK(seuiller),
                     pCtxt);
    // Rajoute tout dans le conteneur vbox.
    gtk_container_add(GTK_CONTAINER(vbox1), hbox1);
    gtk_container_add(GTK_CONTAINER(vbox1), pCtxt->seuil_widget);
    gtk_container_add(GTK_CONTAINER(vbox1), button_seuiller);
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