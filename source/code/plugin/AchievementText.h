#pragma once
#include <cstddef>

// Achievement text in the languages the game itself ships with. The PC release
// carries French, German, Italian and Spanish text banks next to the English
// one, and its font draws their accented letters, so a player running any of
// those sees the gallery in the same language as the rest of the game.
//
// Titles are the trophy names of the PS4 release, as localized there. The
// descriptions are ours: the trophy list states a requirement in a few words,
// while the gallery has room for the whole condition, so each one is written
// out in the language's own words and keeps the terms the trophy text uses --
// the scene names, the difficulty names, "Hoods".
//
// Only characters the game's font can draw are used here: everything stays
// inside the single byte range the text banks themselves use, so no typographic
// dash or curly apostrophe appears below.
struct AchievementLocalizedText
{
    const wchar_t* name;
    const wchar_t* description;
};

// In gallery order, the same order as the PS4 trophy list.
inline constexpr AchievementLocalizedText kGermanAchievementText[] = {
    { L"Psychopath", L"Alle Errungenschaften freischalten." },
    { L"Das hat er nicht kommen sehen", L"Eine Exekution der dritten Brutalitätsstufe ausführen." },
    { L"5-Sterne-Killer", L"Fünf Sterne in einer beliebigen Szene erhalten." },
    { L"Keine Macht den Drogen", L"Eine beliebige Szene auf Hardcore ohne Schmerzmittel abschließen." },
    { L"Knalleffekt", L"Einen Jäger töten, indem ein explosiver Tank beschossen wird." },
    { L"Eiskalt erwischt", L"Einen Jäger mit einem Kühlschrank zerquetschen." },
    { L"Hast du Angst im Dunkeln?", L"Die Leiche eines Opfers im Schatten verstecken." },
    { L"Rosa Nebel", L"Einen Jäger mit einem Kopfschuss aus dem Scharfschützengewehr töten." },
    { L"Mit Köpfchen", L"Einen Jäger mit einem abgetrennten Kopf anlocken." },
    { L"Effizient eliminiert", L"Zwei Jäger mit einem einzigen Schrotflintenschuss töten." },
    { L"Uuh! Uuh! Aah! Aah!", L"Eine beliebige Szene im Affenkostüm spielen." },
    { L"Folge dem weißen Kaninchen", L"Eine beliebige Szene im Kaninchenkostüm abschließen." },
    { L"Tod von hinten", L"Eine beliebige Szene nur mit Exekutionen abschließen." },
    { L"Bleib im Schatten", L"Eine beliebige Szene völlig unentdeckt abschließen." },
    { L"Der Reihe nach ausschalten", L"30 Jäger in der Bonusszene Stahlhart töten." },
    { L"Faustvergnügen", L"In der Bonusszene Prügelspiel überleben, bis 30 Jäger tot sind." },
    { L"Affe entsprungen!", L"Den Zoo in Klappe zu, Affe tot! lebend verlassen." },
    { L"Du gehst nirgendwohin!", L"Alle Hoods in Zeit zu sterben in höchstens sieben Minuten töten." },
    { L"Jägersaison", L"45 Jäger in einer Szene töten." },
    { L"Kopf runter!", L"20 Jäger in einer Szene exekutieren." },
    { L"Stahlhart", L"Mindestens drei Sterne in den Szenen 1 bis 5 erhalten." },
    { L"Prügelspiel", L"Mindestens drei Sterne in den Szenen 6 bis 10 erhalten." },
    { L"Klappe zu, Affe tot!", L"Mindestens drei Sterne in den Szenen 11 bis 15 erhalten." },
    { L"Zeit zu sterben", L"Mindestens drei Sterne in den Szenen 16 bis 20 erhalten." },
    { L"Mörderisch", L"Alle Szenen auf Hardcore abschließen." },
    { L"Gefährlich", L"Alle Szenen auf Fetisch abschließen." },
    { L"4-Sterne-Freak", L"Vier Sterne in jeder Szene auf Fetisch erhalten." },
    { L"5-Sterne-Teufel", L"Fünf Sterne in jeder Szene auf Hardcore erhalten." },
    { L"Die Hände schmutzig machen", L"10 Jäger in einer Szene nur mit den Fäusten töten." },
    { L"Homerun", L"10 Jäger in einer Szene mit dem Baseballschläger töten." },
    { L"Gevatter Tod", L"10 Jäger in einer Szene mit der Sichel töten." },
    { L"Sägewerker", L"Eine Exekution mit der Kettensäge ausführen." },
    { L"Ein besonderes Geschenk", L"Den Kopf des Patrouillenjägers in Rachen des Wahnsinns zum Wachraum bringen." },
};

inline constexpr AchievementLocalizedText kFrenchAchievementText[] = {
    { L"Psychopathe", L"Débloquer tous les succès." },
    { L"Il n'a rien vu venir", L"Réaliser une exécution du troisième niveau de brutalité." },
    { L"Tueur 5 étoiles", L"Obtenir cinq étoiles dans une scène." },
    { L"Les médocs, c'est pour les faibles", L"Finir une scène en Hardcore sans utiliser d'analgésiques." },
    { L"Bim bam boum", L"Tuer un chasseur en tirant sur un réservoir explosif." },
    { L"Le poids du gain", L"Écraser un chasseur avec un réfrigérateur." },
    { L"Qui a peur du noir ?", L"Cacher le corps d'une victime dans l'ombre." },
    { L"En plein dans le mille", L"Tuer un chasseur d'une balle dans la tête au fusil de précision." },
    { L"Délit de faciès", L"Attirer un chasseur avec une tête coupée." },
    { L"Efficacité exemplaire", L"Tuer deux chasseurs d'un seul tir de fusil à pompe." },
    { L"Ouh ouh ah ah", L"Jouer une scène avec le costume de singe." },
    { L"Suivez le lapin blanc", L"Finir une scène avec le costume de lapin." },
    { L"Mort en traître", L"Finir une scène uniquement par des exécutions." },
    { L"Ninja de l'ombre", L"Finir une scène sans être repéré une seule fois." },
    { L"Tous en ligne", L"Tuer 30 chasseurs dans la scène bonus Hard as Nails." },
    { L"Du rififi sur le terrain", L"Survivre jusqu'à la mort de 30 chasseurs dans la scène bonus Brawl Game." },
    { L"La grande évasion", L"Sortir vivant du zoo dans Monkey See, Monkey Die!" },
    { L"On ne bouge plus", L"Tuer tous les Hoods dans Time 2 Die en sept minutes ou moins." },
    { L"La chasse est ouverte", L"Tuer 45 chasseurs dans une seule scène." },
    { L"Qu'on leur coupe la tête !", L"Exécuter 20 chasseurs dans une seule scène." },
    { L"Hard as Nails", L"Obtenir au moins trois étoiles dans les scènes 1 à 5." },
    { L"Brawl Game", L"Obtenir au moins trois étoiles dans les scènes 6 à 10." },
    { L"Monkey See, Monkey Die!", L"Obtenir au moins trois étoiles dans les scènes 11 à 15." },
    { L"Time 2 Die", L"Obtenir au moins trois étoiles dans les scènes 16 à 20." },
    { L"Fou furieux", L"Finir toutes les scènes en Hardcore." },
    { L"Dangereux", L"Finir toutes les scènes en Fetish." },
    { L"Aficionado", L"Obtenir quatre étoiles dans chaque scène en Fetish." },
    { L"Collectionneur", L"Obtenir cinq étoiles dans chaque scène en Hardcore." },
    { L"Les mains dans le cambouis", L"Tuer 10 chasseurs dans une scène à mains nues." },
    { L"Home Run", L"Tuer 10 chasseurs dans une scène à la batte de baseball." },
    { L"La petite faucheuse", L"Tuer 10 chasseurs dans une scène à la faucille." },
    { L"Subtilité", L"Réaliser une exécution à la tronçonneuse." },
    { L"Cadeau", L"Apporter la tête du chasseur en patrouille à la salle des gardes dans Crescendo dans la folie." },
};

inline constexpr AchievementLocalizedText kSpanishAchievementText[] = {
    { L"Psicópata", L"Desbloquea todos los logros." },
    { L"No lo vio venir", L"Realiza una ejecución del tercer nivel de brutalidad." },
    { L"Asesino de 5 estrellas", L"Consigue cinco estrellas en cualquier escena." },
    { L"A tope sin drogas", L"Completa cualquier escena en A saco sin usar analgésicos." },
    { L"Bang, bang, bum", L"Mata a un cazador disparando a un bidón explosivo." },
    { L"Sin grúa no hay paraíso", L"Aplasta a un cazador con una nevera." },
    { L"¿Tienes miedo a la oscuridad?", L"Esconde el cuerpo de una víctima entre las sombras." },
    { L"Niebla rosa", L"Mata a un cazador de un disparo a la cabeza con el rifle de francotirador." },
    { L"Usar la cabeza", L"Atrae a un cazador con una cabeza cortada." },
    { L"Eficiencia enemiga", L"Mata a dos cazadores con un solo disparo de escopeta." },
    { L"¡Ooh, ooh, aah, aah!", L"Juega cualquier escena con el disfraz de mono." },
    { L"Sigue al conejo blanco", L"Completa cualquier escena con el disfraz de conejo." },
    { L"Muerte por la espalda", L"Completa cualquier escena usando solo ejecuciones." },
    { L"Fúndete con las sombras", L"Completa cualquier escena sin que te detecten ni una vez." },
    { L"Alinéalos, derríbalos", L"Mata a 30 cazadores en la escena extra Duro como un clavo." },
    { L"Diversión a puñetazos", L"Sobrevive hasta que mueran 30 cazadores en la escena extra Juego de lucha." },
    { L"Huida simiesca", L"Escapa vivo del zoo en El Pardillo Bueno, ¡El Pardillo Muerto!" },
    { L"¡Tú no vas a ningún sitio!", L"Mata a todos los Hoods en Hora de morir en siete minutos o menos." },
    { L"Temporada de caza", L"Mata a 45 cazadores en una sola escena." },
    { L"¡Que les corten la cabeza!", L"Ejecuta a 20 cazadores en una sola escena." },
    { L"Duro como un clavo", L"Consigue al menos tres estrellas en las escenas 1 a 5." },
    { L"Juego de lucha", L"Consigue al menos tres estrellas en las escenas 6 a 10." },
    { L"El Pardillo Bueno, ¡El Pardillo Muerto!", L"Consigue al menos tres estrellas en las escenas 11 a 15." },
    { L"Hora de morir", L"Consigue al menos tres estrellas en las escenas 16 a 20." },
    { L"Criminal", L"Completa todas las escenas en A saco." },
    { L"Peligroso", L"Completa todas las escenas en Muñequita." },
    { L"Monstruo de 4 estrellas", L"Consigue cuatro estrellas en cada escena en Muñequita." },
    { L"Monstruo de 5 estrellas", L"Consigue cinco estrellas en cada escena en A saco." },
    { L"Ensuciándote las manos", L"Mata a 10 cazadores en una escena usando solo los puños." },
    { L"Bateando para la afición", L"Mata a 10 cazadores en una escena con el bate de béisbol." },
    { L"La parca sombría", L"Mata a 10 cazadores en una escena con la hoz." },
    { L"Asesino silencioso", L"Realiza una ejecución con la sierra eléctrica." },
    { L"Un regalo especial", L"Lleva la cabeza del cazador de patrulla al puesto de guardia en La boca del miedo." },
};

inline constexpr AchievementLocalizedText kItalianAchievementText[] = {
    { L"Psicopatico", L"Sblocca tutti gli obiettivi." },
    { L"Non se n'è nemmeno accorto", L"Esegui un'esecuzione del terzo livello di brutalità." },
    { L"Killer a 5 stelle", L"Ottieni cinque stelle in una scena qualsiasi." },
    { L"Sono pulito", L"Completa una scena qualsiasi in Hardcore senza usare antidolorifici." },
    { L"Bang, Bang, Boom", L"Uccidi un cacciatore sparando a un serbatoio esplosivo." },
    { L"Morte dall'alto", L"Schiaccia un cacciatore con un frigorifero." },
    { L"Hai paura del buio?", L"Nascondi il corpo di una vittima nell'ombra." },
    { L"Nebbia rosa", L"Uccidi un cacciatore con un colpo alla testa col fucile di precisione." },
    { L"Testa a testa", L"Attira un cacciatore con una testa mozzata." },
    { L"Efficienza", L"Uccidi due cacciatori con un solo colpo di fucile a pompa." },
    { L"Ooh, Ooh, Aah, Aah!", L"Gioca una scena qualsiasi con il costume da scimmia." },
    { L"Segui il coniglio bianco", L"Completa una scena qualsiasi con il costume da coniglio." },
    { L"Morte alle spalle", L"Completa una scena qualsiasi usando solo esecuzioni." },
    { L"Resta nell'ombra", L"Completa una scena qualsiasi senza farti mai individuare." },
    { L"Mettili in fila e buttali giù", L"Uccidi 30 cacciatori nella scena bonus Duro come l'acciaio." },
    { L"Che bel divertimento", L"Sopravvivi finché non muoiono 30 cacciatori nella scena bonus Sommossa." },
    { L"Scimmie in fuga", L"Esci vivo dallo zoo in Scimmia vede, scimmia muore!" },
    { L"Non vai da nessuna parte!", L"Uccidi tutti gli Hoods in Ora di morire in sette minuti o meno." },
    { L"Stagione di caccia", L"Uccidi 45 cacciatori in una sola scena." },
    { L"Tagliategli la testa!", L"Esegui 20 esecuzioni in una sola scena." },
    { L"Duro come l'acciaio", L"Ottieni almeno tre stelle nelle scene da 1 a 5." },
    { L"Sommossa", L"Ottieni almeno tre stelle nelle scene da 6 a 10." },
    { L"Scimmia vede, scimmia muore!", L"Ottieni almeno tre stelle nelle scene da 11 a 15." },
    { L"Ora di morire", L"Ottieni almeno tre stelle nelle scene da 16 a 20." },
    { L"Letale", L"Completa tutte le scene in Hardcore." },
    { L"Pericoloso", L"Completa tutte le scene in Fetish." },
    { L"Maniaco a 4 stelle", L"Ottieni quattro stelle in ogni scena in Fetish." },
    { L"Folle a 5 stelle", L"Ottieni cinque stelle in ogni scena in Hardcore." },
    { L"Sporcarsi le mani", L"Uccidi 10 cacciatori in una scena usando solo i pugni." },
    { L"Fuoricampo", L"Uccidi 10 cacciatori in una scena con la mazza da baseball." },
    { L"Il tristo mietitore", L"Uccidi 10 cacciatori in una scena con la falce." },
    { L"Un piccolo macello", L"Esegui un'esecuzione con la motosega." },
    { L"Un dono speciale", L"Porta la testa del cacciatore di pattuglia alla stanza delle guardie in Bacio della follia." },
};

// The gallery's own words, in the same order for every language.
struct GalleryPhrases
{
    const wchar_t* achievements;      // the main menu row and the gallery title
    const wchar_t* achievementsCount; // the same title with "%d/%d" behind it
    const wchar_t* unlocked;          // the line above the name in the popup
    const wchar_t* selectHint;        // keyboard hint: arrows or wheel pick a card
    const wchar_t* pageHint;          // keyboard hint: the page tabs
    const wchar_t* selectAction;      // gamepad hint next to the confirm button
    const wchar_t* pagesAction;       // gamepad hint next to the shoulder buttons
    const wchar_t* backAction;        // gamepad hint next to the back button
};

inline constexpr GalleryPhrases kGermanPhrases = {
    L"ERRUNGENSCHAFTEN", L"ERRUNGENSCHAFTEN %d/%d", L"ERRUNGENSCHAFT FREIGESCHALTET",
    L"Pfeile / Rad: Auswahl", L"Reiter: Seite", L"Auswahl", L"Seiten", L"Zurück",
};

inline constexpr GalleryPhrases kFrenchPhrases = {
    L"SUCCÈS", L"SUCCÈS %d/%d", L"SUCCÈS DÉBLOQUÉ",
    L"Flèches / molette : sélection", L"Onglets : page", L"sélection", L"pages", L"retour",
};

inline constexpr GalleryPhrases kSpanishPhrases = {
    L"LOGROS", L"LOGROS %d/%d", L"LOGRO DESBLOQUEADO",
    L"Flechas / rueda: selección", L"Pestañas: página", L"seleccionar", L"páginas", L"atrás",
};

inline constexpr GalleryPhrases kItalianPhrases = {
    L"OBIETTIVI", L"OBIETTIVI %d/%d", L"OBIETTIVO SBLOCCATO",
    L"Frecce / rotellina: selezione", L"Schede: pagina", L"seleziona", L"pagine", L"indietro",
};
