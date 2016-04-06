/*******************************************************************
 * Ultra Splitter
 *
 * Programme pour des fichiers en plusieurs morceaux. 
 *
 * -----------------------------------------------------------
 * Ceci est la mise à jour d'un de mes anciens programmes DOS
 * fait en QuickBasic.
 * -----------------------------------------------------------
 *
 * Auteur:  Asher256
 * Contact: asher256@gmail.com
 *
 *------------------------------------------------------------------
 * Ce code source est distribué selon les termes de la licence
 * GNU General Public Licence v2 ou ultérieure (selon votre
 * convenance). Veuillez lire le fichier COPYING.txt ou
 * COPYING-FR.txt pour plus d'informations.
 *------------------------------------------------------------------
 *
 *******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "interface.h"
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Progress.H>
#include <FL/x.H>

#include "Fl_Nice.H"

//quelques defines pour la portabilité
#ifdef LINUX
	#define C_RACINE '/'
	#define S_RACINE "/" 
#else
	#ifdef WIN32
		#define C_RACINE '\\'
		#define S_RACINE "\\"
	#else
		#error Vous devez spécifier le define LINUX ou WIN32...
	#endif
#endif

//=============> les tailles
#define MO(n) ((long)(n*1024*1024)) //conv mo en o
#define KO(n) ((long)(n*1024))      //conv ko en o

typedef struct {
	char *str;     //sa chaine
	long n;        //la taille en octets
} TAILLE;

//le dernier size doit avoir zero comme
//valeur
#define DEFAULT_TAILLE_MANUELLE "1.38" // 1.38 mo
TAILLE taille[] = {
	{"1.38 MO",  MO(1.38)},
	{"650 ko",   KO(650)},
	{"1.44 MO",  MO(1.44)},
	{"720 ko",   KO(720)},
	{"8 MO",     MO(8)},
	{"64 MO",    MO(64)},
	{"128 MO",   MO(128)},
	{"256 MO",   MO(256)},
	{"650 MO",   MO(650)},
	{0,0}
};

//les variables(suite aprŠs la classe)
UserInterface *I=0;
Fl_Window *main_window=0;
Fl_Window *about=0;

//quand on change de mode: division/rassemblement
//alors on sauvegarde ici le précédent chemin ;)
char *precedent_fichier=0;
char *precedent_chemin=0;

// Fichier de configuration
// ces variable sont chargés depuis le fichier de configuration
char *cfg_precedent_fichier=0;
char *cfg_precedent_chemin=0;
char *cfg_precedent_fichier2=0; //pour le mode rassemblement
char *cfg_precedent_chemin2=0;

// le progress bar
class Fl_Progress_Window : public Fl_Window
{
	public:
		Fl_Progress *progress1;
		Fl_Progress *progress2;
		Fl_Box *progress_filename;

		void value(int percent1, int percent2, const char *filename=0);
		int handle(int event);
		void show();

		//constructeur
		Fl_Progress_Window()  : Fl_Window(238,105,"Progression...")
		{
				// Progress 1 Label
				Fl_Box *label = new Fl_Box(10,10,215,15,"Pourcentage Général:");
				label->labelsize(12);
				label->labelfont(1);
				label->labelcolor((Fl_Color)136);
				label->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

				//Progress1
				progress1 = new Fl_Progress(10,28,215,20,"0%");
				progress1->selection_color((Fl_Color)175);
				progress1->maximum(100);
				progress1->minimum(0);
				progress1->labelsize(12);

				// Progress filename (label progress 2)
				progress_filename = new Fl_Box(10,52,215,15, "Fichier:");
				progress_filename->labelsize(12);
				progress_filename->labelfont(1);
				progress_filename->labelcolor((Fl_Color)136);
				progress_filename->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

				// Progress 2
				progress2 = new Fl_Progress(10,70,215,20,"0%");
				progress2->maximum(100);
				progress2->minimum(0);
				progress2->labelsize(12);
				progress2->selection_color((Fl_Color)175);

				//fin du window...
				this->set_modal();
				this->end();
		}
};

//afficher cette fenêtre...
void Fl_Progress_Window :: show()
{
	this->position(
					main_window->x() + (main_window->w() - this->w())/2,
					main_window->y() + (main_window->h() - this->h())/2
				  );

	Fl::check();
	Fl_Window::show();
	Fl::check();
}


//le handle...
int Fl_Progress_Window::handle(int event)
{
		if(event==FL_KEYBOARD) return 1;   //pas de keyboard...
		return Fl_Window::handle(event);  //sinon, le handle par défaut...
}

// mets une valeur au window (puis redraw!)
void Fl_Progress_Window::value(int percent1, int percent2, const char *filename)
{
		static char string[60];
		static char string2[60];
		char modif=0;

		//filename
		if(filename) {
				this->progress_filename->label(filename);
				modif=-1;
		}

		//progress 1
		if(percent1>=0 && percent1!=(int)this->progress1->value()) { //si par ex -1 alors pas changer
			sprintf(string,"%i%%",percent1);
			this->progress1->label(string);
			this->progress1->value((float)percent1);
			modif=-1;
		}

		//progress 2
		if(percent2>=0 && percent2!=(int)this->progress2->value()) {
			sprintf(string2,"%i%%",percent2);
			this->progress2->label(string2);
			this->progress2->value((float)percent2);
			modif=-1;
		}

		if(modif) Fl::check();
}

//fonctions
void rassembler_click(Fl_Widget *);

//quelsues variables (il y en a d'autres avant la classe)
//j'ai mis les autres là bas car la classe a besoin d'eux...
Fl_Progress_Window progress;

//les sizes
//par défaut

// Si version WINDOWS alors centrer la fenêtre!
// (lol)
void center(Fl_Window *window)
{
window->position(
/* x */			
Fl::x()+
(Fl::w()-window->w())/2

/* y */ ,
Fl::y()+
(Fl::h()-window->h())/2)
;
}

//si un fichier existe 
int exists(const char *path)
{
	FILE *file = fopen(path,"rb");
	if(!file) return 0;
	fclose(file);
	return 1;
}

//conv fichier en répertoire, en mettant un null devant le dernier slash
//trouvé!
void file_to_dir(char *path)
{
	char *s;
	s=path+strlen(path);
	while(s!=path) {
		//il a trouvé le SLASH !!!!
		//donc c un chemin!
		if(*s=='/' || *s=='\\') { 
				*s=0; 
#ifdef WIN32
				//si c'est le lecteur 
				//p.ex: C: sans le antislash à la fin
				//alors...
				if(strlen(path)==2 && path[1]==':') 
						strcat(path,"\\");
#endif
				return; 
		}
		s--;
	}
	//si pas trouvé le slash alors c un fichier
	//sans chemin!
	path[0]=0;
}
	
//mets le fichier à diviser dans l'Input
//et mets le répertoire de ce fichier dans chemin
//cette fonction modifie s en le convertissant en répertoire...
//et en convertissant / par \ sous windows
void set_filename(char *s)
{
#ifdef WIN32
	char *s2;
	// conv / en \ (::)
	s2=s;
	while(*s2) {
			if(*s2=='/') *s2='\\';
			s2++;
	}
#endif

	I->fichier->value(s);
	file_to_dir(s);
	if(!*s) 
		I->chemin->value("." S_RACINE);
	else
		I->chemin->value(s);
}

const char *octet_to_string(long octet)
{
	static char str[256];
	if(octet>1024*1024) 
		sprintf(str,"%.2f Mo",(double)octet/(1024*1024));
	else if(octet>1024) 
		sprintf(str,"%.2f Ko",(double)octet/1024);
	else
		sprintf(str,"%i Octets",(int)octet);
	return str;
}

void update_taille_fichier(const char *s)
{
	FILE *file = fopen(s,"r");
	if(file) {
		fseek(file,0,SEEK_END);
		long fsize=ftell(file);
		fclose(file);
		if(fsize>0) 
			I->taille_fichier->label(octet_to_string(fsize));
	}
}

//retourne le répertoire par défaut
//(pour la boite de dialogue...)
//normalement fichier->value()
//mais si fichier->value() est vide
//alors il mets le précédent chemin
const char *get_precedent_chemin()
{
	if(*(char*)I->chemin->value())
			return I->chemin->value();
	else {
			if(precedent_chemin && *precedent_chemin)
					  return precedent_chemin;
			//normalement il envoie les trucs de rassemblement
			else if(cfg_precedent_chemin)
						return cfg_precedent_chemin;
			// si mode RASSEMBLEMENT
			else 
				return (cfg_precedent_chemin2)?cfg_precedent_chemin2:"";
	}
	return "";
}
const char *get_precedent_fichier()
{
	if(*((char *)I->fichier->value())) 
			return I->fichier->value();
	else {
		if(precedent_fichier && *precedent_fichier)
			  return precedent_fichier;

		//MOT RASSEMBLEMENT
		if(I->rassembler->value()) {
			if(cfg_precedent_fichier2) 
				return cfg_precedent_fichier2;
		}
		else
		//si mode rassemblement
		return (cfg_precedent_fichier)?cfg_precedent_fichier:"";
	}
	return "";
}

//callback
//chercher un fichier
void btn_browse_file_click(Fl_Widget *)
{
	const char *s;

	//le titre
	char *title = "Sélectionnez le fichier à diviser...";

	//les patterns
	static FN_PATTERN pattern_rassembler[] = { {"Première Division","u001"}, {0,0} };
	FN_PATTERN *pattern=0; //par défaut... rien! tous les fichier!

	//mets le pattern de rassembler s'il le faut
	if(I->rassembler->value()) {
		title="Sélectionnez un fichier u001 pour le rassemblement...";
		pattern=pattern_rassembler;
	}
	
	//ouvre le file chooser
	if((s=fn_file_chooser(title, pattern, get_precedent_fichier()))) {
		if(!exists(s)) {
			fn_alert("Le fichier\n%s\nn'existe pas...",s);
			return;
		}
		
		//mets un label vide..;
		I->taille_fichier->label("");
		
		//mets le fichier dans le text output
		char *str = strdup(s);
		if(!str) return;
		set_filename(str);
		free(str);

		//mets la taille dans le label taille_fichier
		{
			//si rassembler ne mets pas le size...
			if(I->rassembler->value()) return;

			//fonction pour me
			//ttre le size du fichier
			//dans le label
			update_taille_fichier(s);
		}
	}
}

//callback
//chercher un fichier
void btn_browse_dir_click(Fl_Widget *)
{
	const char *s;
	if((s=fn_dir_chooser("Sélectionnez le dossier...",get_precedent_chemin()))) 
		I->chemin->value(s);
}

//callback
// quitter
void btn_quitter_click(Fl_Widget *)
{ exit(0); }

//get arguments
void get_arguments(int argc, char **argv)
{
	//options
	if(argc>1) {
		FILE *f = fopen(argv[1],"rb");
		if(!f) 
			fn_alert("Erreur, le programme ne peux ouvrir le fichier:\n%s\n(passé en arguments...)",argv[1]);
		else {
			fclose(f);

			//Changer de mode si extention u001
			const char *ext = fl_filename_ext(argv[1]);
			if(strcmp(ext,".u001")==0) 
				I->rassembler->value(1); //change de mode...
			
				
			//mets les attributs (si changés par rassembler->value(1)
			rassembler_click(0);

			//ici, le mettre dans le textbox
			char str[1024];
			//convertissement en chemin complet!
			fl_filename_absolute(str,1023,argv[1]);
			//update taille fichier seulement si mode division
			if(I->rassembler->value()==0) update_taille_fichier(str);
			//mets str dans I->str
			set_filename(str);
		}
	}
	else 
		rassembler_click(0);
}

//pour afficher about
void show_about(Fl_Widget *)
{
	about->position(main_window->x()+50,main_window->y()+50);
	about->show();
	Fl::wait();
}

void about_click(Fl_Widget *)
{
	about->hide();
}

long get_size()
{
	//si taille prédéfinie
	if(I->check1->value()) 
		return taille[I->combo_taille->value()].n;
	else
	//si non prédéfinie
	{
		const char *s = I->taille->value();
		int i = I->combo_mesure->value();

		while(*s) { 
			if(i==2 && *s=='.') {
				fn_alert("Attention, pas de virgule dans les octets...");
				return 0;
			}
			if(!isdigit(*s) && *s!='.') {
				fn_alert("Le chiffre \"%s\" est incorrecte...\n",I->taille->value());
				return 0; 
			}
			s++; 
		}
		
		//maintenant retourne la bonne valeur
		switch(i) {
			case 0: //MO
				return (long)MO(atof(I->taille->value()));
			case 1: //KO
				return (long)KO(atof(I->taille->value()));
			case 2: //Octet
				return (long)atof(I->taille->value());
		}
	}
	return 0;
}

// splitter le tout !
#define buffer_size 32768
void diviser()
{
	long size = get_size();
	char buffer[buffer_size];
	long i;

	//erreur dans la taille?
	if(size==0) { return; }


	FILE *file = fopen(I->fichier->value(),"rb");
	if(!file) {
		fn_alert("Erreur, le programme ne peux ouvrir le fichier \"%s\" en lecture...\n",I->fichier->value()); 
		return;
	}

	//calcule la taille du fichier
	if(fseek(file,0,SEEK_END)) { fn_alert("Erreur, le programme ne peux se positionner dans le fichier..."); fclose(file); return; }
	long fsize=ftell(file); 
	if(fsize==0) { fn_alert("Erreur, le programme ne peux lire la taille du fichier..."); fclose(file); return; }
	rewind(file); //reviens à la position initiale

	//calcule le nombre de split
	long max_split = (long)(fsize/size) + (((fsize%size)!=0)?1:0);

	//de 1 à 999, sinon err
	if(max_split>999) {
		fn_alert("Erreur, vous avez dépassé le seuil de 999 divisions...\n");
		fclose(file);
		return;
	}

	//les erreurs...
	if(max_split<=1) {
		fn_message("Après le calcul, la taille de ce fichier est de %s. Elle ne dépasse même pas une division.",octet_to_string(fsize));
		fclose(file);
		return;
	}
	if(fsize<=size) {
		fn_alert("La taille du fichier \"%s\" qui est %s est inférieure de la taille d'une seule division!",I->fichier->value(),octet_to_string(fsize));
		fclose(file);
		return;
	}
	if(!fn_ask("Vous êtes sur le point de diviser le fichier:\n\n\"%s\"\n\nEn %i parties... Continuer?",I->fichier->value(),max_split)) {
		fclose(file);
		return;	
	}

	//variable ou il y a le nom du fichier qui est divisé...
	const char *name = fl_filename_name(I->fichier->value());
	char *path = (char *)malloc(strlen(I->chemin->value())+1+strlen(name)+5+1);
	strcpy(path,I->chemin->value());

	//ajouter / à la fin
	{
		char *str=path+strlen(path);
		while(*str!=C_RACINE) {
			if(!isspace(*str) && *str!=C_RACINE) { 
				strcat(path,S_RACINE);
				break;
			}
			str--;
		}
	}
	
	strcat(path,name);
	char *path_end = path+strlen(path);

	//supprimer les fichier *.00* qui existent
	for(i=0;i<max_split;i++) {
		sprintf(path_end,".u%03li",i+1);
		FILE *filed = fopen(path,"rb");
		if(filed) {
			fclose(filed);
			if(!fn_ask("Le programme a trouvé quelques anciennes divisions du fichier à diviser.\n\npar exemple:\n\"%s\"\n\nCes fichiers doivent être supprimés...\n\nLe souhaitez vous?",path,I->fichier->value())) {
				fn_alert("Si ces fichiers de divisions existent encore le\nprogramme ne peux diviser le fichier...");
				fclose(file); free(path);
				return;
			}

			//supprime tous les fichiers
			for(long i=0;i<1000;i++) {
				sprintf(path_end,".u%03li",i+1);
				remove(path);
			}
			fn_message("Les fichiers sont maintenant supprimés.!\n\nAppuyez sur OK pour divisier le fichier...");
			break;
		}
	}

	progress.show();
	progress.value(0,0,"");
	

	//division !
	for(i=0;i<max_split;i++) {
		//introduction
		sprintf(path_end,".u%03li",i+1);

		FILE *filed = fopen(path,"wb");
		if(!filed) {
			fn_alert("Erreur dans l'ouverture du fichier \"%s\" en écriture...",path);
			fclose(file); 
			fclose(filed); 
			for(long j=0;j<=i;j++) { sprintf(path_end,".%03li",j+1); remove(path);  } free(path);
			progress.hide();
			return;
		}

		{
		//conv le path en un nom de fichier
		const char *s = strrchr(path,C_RACINE);
		if(!s) s=path; else s++;
		progress.value(-1,-1,s); 
		}

		//traiitteeeeeement !!
		size_t readed=0;
		int actual_percent=0;
		long split_size=(i==max_split-1)?(fsize-((max_split-1)*size)):size;
		double n,d;
		for(long j=split_size;j>0;j-=readed) {
			//calcul actual percent
			//split_size      ----> 100%
			//split_size-j    ----> ?

			//change le value du progress...
			n = split_size-j;
			d = split_size;
			actual_percent = (int)((n*100)/d);

			//met les nouvelles valeurs dans le progress
			{
			n=((i*100)+actual_percent);
			d=max_split;
			progress.value( 
				(int)(n/d)
				,actual_percent);
			}

			//pour toujours afficher le progress
			if(!progress.shown()) progress.show();

			// === Fin calcul ====

			size_t read_size = (j<buffer_size)?j:buffer_size;
			readed=fread(buffer,1,read_size,file);
			if(readed==0) break;

			size_t writed = fwrite(buffer,readed,1,filed);
			
			if(writed<1) {
				fn_alert("Erreur dans l'écriture dans le fichier \"%s\"...\nPeu être pas assez d'espace?\nOu disque protégé en écriture?...",path);
				fclose(file); fclose(filed); for(long j=0;j<=i;j++) { sprintf(path_end,".%03li",j+1); remove(path);  } free(path);
				progress.hide();
				return;
			}

		}

		progress.value(-1,100);

		fclose(filed);
	}
	
	progress.value(100,100);

	//et enfin, ferme le fichier...
	fclose(file);
	
	fn_message("La division du fichier\n\n\"%s\"\n\nen %i parties réussie avec succès !",I->fichier->value(),max_split);

	//fin !
	progress.hide();
	
	//désalloque path
	free(path);
}

//pour rassembler les fichiers!
//(le mode 2)
void rassembler()
{
	char buffer[buffer_size];

	//teste l'existance
	if(!exists(I->fichier->value())) {
		fn_alert("Le fichier: \n\"%s\"\nn'existe pas...",I->fichier->value());
		return;
	}

	//teste l'extention
	{const char *ext = fl_filename_ext(I->fichier->value());
	if(!ext || strcmp(ext,".u001")) {
		fn_alert("L'extention du fichier:\n\"%s\"\ndoit être u001 (première division)...",I->fichier->value());
		return;
	}}

	//crée quelques variables importantes
	int max_split=0;
	char *path = strdup(I->fichier->value()); if(!path) {fn_alert("Pas assez de mémoire...");return;}
	if(!path) {fn_alert("Pas assez de mémoire...");return;}
	char *new_path = 0; //il est mis après le test
	char *path_end = path+strlen(path)-5; 

	//teste 'il y a un vide
	//maintenant tester si tous les fichier *.00* existent
	//et que rien ne manque
	//par ex s'il y a u001 002 puis 004 alors erreur car
	//003 n'existe pas...
	int i;
	for(i=1;i<=999;i++) {
		sprintf(path_end,".u%03i",i);
		if(exists(path)) 
			max_split=i;
		else
			break;
	}
	if(max_split<1) {
			fn_alert("Le programme ne peux rassembler moins d'une division...");
			free(path);
			return;
	}
	if(max_split==1) {
		fn_alert("Le programme n'a trouvé que le fichier u001.\nIl faut au moins deux divisions pour faire un rassemblement...");
		free(path);
		return;
	}
	i++; //va vers le prochain carac
	for(;i<=999;i++) {
		sprintf(path_end,".u%03i",i);
		if(exists(path)) {
			fn_alert("Le programme a trouvé un vide entre le fichier ayant l'extention u%03i et u%03i...\nIl y a probablement des fichiers qui manquent!",max_split,i);
			free(path);
		}
	}

	path_end[0]=0; //enlève le .00x pour tester l'existance
	//et aussi pour créer new_path
	//l'existance c'est après new_path

	//CONVERTISSEMENT DE path en new_chemin/basename'path'
	//mets le chemin selon chemin->value()
	//
	//le résultat est dans new_path
	{
			char *slash = strrchr(path,C_RACINE);

			if(!slash) 
					slash=path; // par exemple asher.fl :)
			else
					slash++; //passe le slash pour n'avoir que le BASENAME

			//allocation d'un new path
			new_path = (char *)malloc(strlen(I->chemin->value())+1+strlen(slash)+1);
			if(!path) { fn_alert("Pas assez de mémoire..."); free(path); return; }

			//mets le chemin
			char *strSlash = S_RACINE;
			if(strlen(I->chemin->value())>0 && I->chemin->value()[strlen(I->chemin->value())-1]==C_RACINE)
					strSlash="";
			sprintf(new_path,"%s%s%s",I->chemin->value(),strSlash,slash);
	}
	
	//demande s'il veut continuer
	if(!fn_ask("Vous êtes sur le point de rassembler %i fichiers dans:\n\n\"%s\"\n\nCe fichier%ssera remplacé par le contenu des divisions.\nSouhaitez vous continuer?",max_split,new_path,exists(new_path)?" EXISTE et ":" ")) {
		free(path); free(new_path);
		return;
	}

	//ouvre le fichier destination
	FILE *file = fopen(new_path,"wb");
	if(!file) {
		fn_alert("Le fichier: \"%s\"\n\nne peux être ouvert en écriture...",new_path);
		remove(new_path); //supprime le fichier ou l'on va rassembler
		free(path); free(new_path);
		return;
	}

	progress.show();
	progress.value(0,0,"");

	//parcours les maxsplit
	for(i=1;i<=max_split;i++) {
		static size_t readed;
		sprintf(path_end,".u%03i",i);

		//met le nom du fichier
		{
			char value[30];
			sprintf(value,"Ajout de la %i%s division...",i,(i==1)?"ère":"ème");
			progress.value(-1,-1,value);
		}

		//ouverture
		FILE *file_src = fopen(path,"rb");
		if(!file_src) {
			fn_alert("Le fichier \"%s\"\n\nne peux être ouvert en lecture...\n\nL'opération ne peux continuer...\n",path);
			*path_end=0;
			remove(new_path); //supprime le fichier ou l'on va rassembler
			free(path); free(new_path);
			fclose(file);
			return;
		}

		//calcule le size
		fseek(file_src,0,SEEK_END);
		long fsize=ftell(file_src);
		long fsize_writed=0;
		rewind(file_src);
		
		//transfers
		int percent;
		float n,d;
		while(!feof(file_src)) {
			//lecture
			readed=fread(buffer,1,buffer_size,file_src);
			if(!readed) break;

			fsize_writed+=(long)readed; //pour le calcul du pourcentage
			
			//écriture
			if(fwrite(buffer,readed,1,file)<1) {
				fn_alert("Erreur dans l'écriture dans le fichier:\n\"%s\"\nDisque plein ou protégé en écriture?",path);
				*path_end=0;
				remove(new_path); //supprime le fichier ou l'on va rassembler
				free(path); free(new_path);
				fclose(file); fclose(file_src);
				return;
			}

			//calcul pourcentage normal
			// fsize          ----> 100
			// fsize_writed

			//calcul du pourcentage !!
			n=fsize_writed;
			d=fsize;
			percent = (int)((n*100)/d);

			//encore
			n=((i-1)*100)+percent;
			d=max_split;

			//ce qui est en bas est déjà fait!
			progress.value((int)(n/d),percent);
		}

		//close
		fclose(file_src);
	}
	
	fclose(file);
	fn_message("L'opération de rassemblement finie avec succès!");

	progress.hide();

	if(fn_ask("voulez vous supprimer les fichiers des divisions de u001 à u%03i?",max_split)) {
		int err=0;
		for(int i=1;i<=max_split;i++) {
			sprintf(path_end,".u%03i",i);
			if(remove(path)) err=1;
		}

		if(err) 
			fn_message("Les fichiers ont été supprimés, bien que\ncertains n'ont pas pu l'être!");
		else 
			fn_message("Les fichiers ont été supprimés avec succès!");
	}

	//désallocation
	free(path);
	free(new_path);
}

void btn_split_click(Fl_Widget *)
{
	// si il n'as mis aucun fichier
	if(!*I->fichier->value()) {
		fn_alert("Vous devez avant tout sélectionner un fichier...");
		return;
	}
	//**** commence le split
	if(!fl_filename_isdir(I->chemin->value())) {
		fn_alert("Le répertoire \"%s\" est invalide...",I->chemin->value());
		return;
	}

	if(I->rassembler->value()) 
		rassembler();
	else 
		diviser();
}

//si l'on clique sur rassembler
void rassembler_click(Fl_Widget *)
{
	//sauvegarde les valeurs actuelles
	char *p_fichier=strdup(I->fichier->value());
	char *p_chemin=strdup(I->chemin->value());

	//si pas assez de memoire...
	if(!p_fichier || !p_chemin) {
			fn_alert("Pas assez de mémoire...");
			exit(1);
	}

	//mets les anciennes valeurs
	if(precedent_fichier) I->fichier->value(precedent_fichier); else I->fichier->value("");
	if(precedent_chemin)  I->chemin->value(precedent_chemin);   else I->chemin->value("");

	//maintenant crée mets les saves des paramètres actuels
	//dans les variables pour le prochain changement!
	precedent_fichier=p_fichier;
	precedent_chemin=p_chemin;

	// RASSEMBLEMENT
	if(I->rassembler->value()) {
		//labels et tooltip
        I->fichier->label("Fichier u001:");
		I->chemin->label("Chemin du rassemblement:");
		I->btn_split->label("&Rassembler");
		I->btn_split->tooltip("Rassembler un groupe de fichiers en un fichier\nunique en se basant sur\nle premier fichier *.u001...");

		//couleurs
		I->fichier->labelcolor(58);
		I->chemin->labelcolor(58);
		I->btn_split->color(58);
		I->btn_split->selection_color(58);
		I->titre->labelcolor(58);

		//disable
		I->check1->deactivate();
		I->check2->deactivate();
		I->combo_taille->deactivate();
		I->combo_mesure->deactivate();
		I->taille->deactivate();
		I->taille_fichier->hide();
		I->label_taille_division->deactivate();
		I->label_taille_division->labelcolor(58);
	}

	// DIVISION
	else {
		//labels et tooltip
		I->fichier->label("Fichier à diviser:");
		I->chemin->label("Chemin des divisions:");
		I->btn_split->label("&Diviser");
		I->btn_split->tooltip("Diviser un fichier en plusieurs morceaux...");

		I->fichier->labelcolor(136);
		//couleurs
		I->fichier->labelcolor(136);
		I->chemin->labelcolor(136);
		I->btn_split->color(136);
		I->btn_split->selection_color(136);
		I->titre->labelcolor(136);

		//enabled
		I->check1->activate();
		I->taille->activate();
		I->check2->activate();
		I->combo_taille->activate();
		I->combo_mesure->activate();
		I->taille_fichier->show();
		I->label_taille_division->activate();
		I->label_taille_division->labelcolor(136);
	}
	main_window->redraw();
}

void save_config();

void bye()
{
	static int first_call=1;

	if(first_call) {
		save_config();

		//free les fenêtres
		delete I;
		delete main_window;
		delete about;

		//free toutes les var
		free(precedent_chemin);      free(precedent_fichier);
		free(cfg_precedent_chemin);  free(cfg_precedent_fichier);
		free(cfg_precedent_chemin2); free(cfg_precedent_fichier2);

		//c'est le dernier bye appelé:
		first_call=0;
	}
}

/*****************************
 * charger et sauver
 * la configuration
 ****************************/
#define VENDOR "Asher256"
#define NAME   "UltraSplitter"
#define ROOT   Fl_Preferences::USER
void save_config()
{
		const char *f1,*c1,*f2,*c2;
		if(!I || !main_window) return;

		// ouvrir le fichier de configuration
		Fl_Preferences *p=new Fl_Preferences(ROOT,VENDOR,NAME);

		//DIVISION
		if(!I->rassembler->value()) {
			f1 = (*I->fichier->value())?I->fichier->value():cfg_precedent_fichier;
			c1 = (*I->chemin->value())?I->chemin->value():cfg_precedent_chemin;
			f2 = (precedent_fichier && *precedent_fichier)?precedent_fichier:cfg_precedent_fichier2;
			c2 = (precedent_chemin && *precedent_chemin)?precedent_chemin:cfg_precedent_chemin2;
		}
		//RASSEMBLEMENT
		else {
			f1 = (precedent_fichier && *precedent_fichier)?precedent_fichier:cfg_precedent_fichier;
			c1 = (precedent_chemin && *precedent_chemin)?precedent_chemin:cfg_precedent_chemin;
			f2 = (*I->fichier->value())?I->fichier->value():cfg_precedent_fichier2;
			c2 = (*I->chemin->value())?I->chemin->value():cfg_precedent_chemin2;
		}

		//teste s'il y a eu des zéros..
		//il se peut que par exemple il n'ait pas
		//chargé du fichier cfg la variable ou encore...
		//qu'il n'ait pas sauvé le précédent fichier...
		if(!f1) f1="";
		if(!c1) c1="";
		if(!f2) f2="";
		if(!c2) c2="";

		//mets les informations
		p->set("Division/precedent_fichier",f1);
		p->set("Division/precedent_chemin",c1);
		p->set("Rassemblement/precedent_fichier",f2);
		p->set("Rassemblement/precedent_chemin",c2);

		delete p;
}

void load_config()
{
		// ouvrir le fichier de configuration
		Fl_Preferences *p=new Fl_Preferences(ROOT,VENDOR,NAME);

		// prendre les informations
		p->get("Division/precedent_fichier",cfg_precedent_fichier,"");
		p->get("Division/precedent_chemin", cfg_precedent_chemin,"");
		p->get("Rassemblement/precedent_fichier",cfg_precedent_fichier2,"");
		p->get("Rassemblement/precedent_chemin", cfg_precedent_chemin2,"");

		delete p;
}

// Main
int main(int argc, char **argv)
{
	atexit(bye);
#ifdef LINUX
	fl_open_display();
#endif

	//nouvelle userinterface
	I=new UserInterface;

	//créer et centrer le window principal
	main_window = I->make_window();
	center(main_window);

	//about
	about = I->make_about();
	I->about_button->callback(about_click);

	//construction des combos

	//taille automatique qui se trouvent dans taille[]
	//les ajoutent dans combo
	{
		int i=0;
		while(taille[i].str) {
			I->combo_taille->add(taille[i].str);
			i++;
		}
	}

	//sélectionner la première taille automatique
	I->combo_taille->value(0);

	//les tailles manuelles
	I->combo_mesure->add("Mo");
	I->combo_mesure->add("Ko");
	I->combo_mesure->add("Octet");
	I->combo_mesure->value(0);
	I->taille->value(DEFAULT_TAILLE_MANUELLE);

	//callback
	I->btn_quitter->callback(btn_quitter_click);
	main_window->callback(btn_quitter_click);
	I->btn_browse_file->callback(btn_browse_file_click);
	I->btn_browse_dir->callback(btn_browse_dir_click);
	I->btn_about->callback(show_about);
	I->btn_split->callback(btn_split_click);
	I->rassembler->callback(rassembler_click);

	//les arguments passés au programme
	get_arguments(argc,argv);

	//Charger le fichier de configuration...
	load_config();

	//show
	Fl::visual(FL_DOUBLE|FL_INDEX);
	{
	char *_argv[] = {argv[0],0};
	main_window->show(1,_argv);
	}
	return Fl::run();
}


