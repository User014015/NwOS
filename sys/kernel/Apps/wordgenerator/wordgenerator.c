#include "wordgenerator.h"

#include "../../kernel.h"


/* =========================================================
   Kernel functions
   ========================================================= */

void print(const char* text);
void read_line(char* buffer, int max);

int random_range(int min, int max);


/* =========================================================
   Words
   ========================================================= */

static const char* words[] =
{

    "hello ",
    "bye ",
    "car ",
    "why ",
    "later ",
    "wait ",
    "me ",
    "i am ",
    "World ",
    "keep ",
    "again ",
    "great ",
    "Autobus ",
    "House ",
    "We ",
    "good ",
    "cosmos ",
    "kernel ",
    "program ",
    "Hour ",
    "NwOS ",
    "Sky ",
    "Cat ",
    "Pilot ",
    "Adam ",
    "Skyliner ",
    "god ",
    "Again ",
    "Retry ",
    "Elephant ",
    "New year ",
    "When ",
    "Where ",
    "Dog ",
    "Happy ",
    "Who ",
    "Sorry ",
    "Telephone ",
    "Main ",
    "Day ",
    "Always ",
    "Meow ",
    "I need ",
    "I not ",
    "School ",
    "Who i ",
    "Am ",
    "Little ",
    "You ",
    "Are ",
    "Apple ",
    "computer ",
    "machine ",
    "robot ",
    "digital ",
    "signal ",
    "window ",
    "river ",
    "tree ",
    "rain ",
    "fire ",
    "light ",
    "shadow ",
    "sound ",
    "music ",
    "coffee ",
    "friend ",
    "game ",
    "code ",
    "byte ",
    "memory ",
    "screen ",
    "keyboard ",
    "mouse ",
    "planet ",
    "moon ",
    "star ",
    "future ",
    "today ",
    "morning ",
    "night ",
    "Reality? ",
    "Big ",
    "Is you ",
    "me ",
    "Whose ",
    "Pizza ",
    "Drawing ",
    "Mooner ",
    "Computing ",
    "Like ",
    "Boot ",
    "Humanoid ",
    "Human ",
    "God ",
    "Panic ",
    "Were ",
    "Count ",
    "Word ",
    "Infinity ",
    "Plus ",
    "Minus ",
    "system ",
    "Calculating ",
    "All space ",
    "You are ",
    "There is ",
    "Not a ",
    "So much ",
    "Bananas ",
    "People ",
    "Hating ",
    "Loving ",
    "Love ",
    "Hate ",
    "Okay ",
    "Desert ",
    "Bad ",
    "Yes ",
    "No ",
    "abandon ",
    "abandoned ",
    "abandonedly ",
    "abase ",
    "abased ",
    "abhor ",
    "about ",
    "above ",
    "absurd ",
    "act ",
    "action ",
    "actual ",
    "acute ",
    "ad ",
    "adam ",
    "adapt ",
    "adapted ",
    "add ",
    "added ",
    "addition ",
    "administration ",
    "adopted ",
    "adopt ",
    "advice ",
    "afar ",
    "affected ",
    "effects ",
    "advance ",
    "ages ",
    "age ",
    "afford ",
    "agents ",
    "agito ",
    "africa ",
    "cairo ",
    "all ",
    "allay ",
    "alleging ",
    "allow ",
    "alloy ",
    "ago ",
    "am ",
    "amazed ",
    "always ",
    "ambition ",
    "ambitions ",
    "amazing ",
    "amiss ",
    "among ",
    "an ",
    "analyzed ",
    "animals ",
    "animal ",
    "anniversary ",
    "animate ",
    "any ",
    "arguments ",
    "argument ",
    "bad ",
    "back ",
    "backs ",
    "bald ",
    "ball ",
    "balaneion ",
    "banner ",
    "barbarian ",
    "bare ",
    "bared ",
    "babe ",
    "baby ",
    "awful ",
    "babylon ",
    "battle ",
    "battles ",
    "bear ",
    "beast ",
    "beasts ",
    "bathing ",
    "beaten ",
    "bee ",
    "bees ",
    "bin ",
    "bird ",
    "bind ",
    "binary ",
    "birth ",
    "bishop ",
    "cares ",
    "cannot ",
    "canst ",
    "capital ",
    "captive ",
    "carefully ",
    "caresses ",
    "case ",
    "cases ",
    "casting ",
    "carcase ",
    "catholic ",
    "catholics ",
    "casts ",
    "cattle ",
    "cd ",
    "cease ",
    "charity ",
    "cheap ",
    "chain ",
    "centre ",
    "central ",
    "crage ",
    "climb ",
    "clear ",
    "cleared ",
    "cleave ",
    "close ",
    "closed ",
    "closes ",
    "clothing ",
    "cleaving ",
    "clothed ",
    "cloud ",
    "clouds ",
    "colourfully ",
    "color ",
    "colors ",
    "comforted ",
    "combined ",
    "comfortest ",
    "commander ",
    "commanded ",
    "command ",
    "commend ",
    "crown ",
    "damage ",
    "damaged ",
    "damaging ",
    "dangerous ",
    "dares ",
    "dared ",
    "dark ",
    "dashed ",
    "data ",
    "dates ",
    "david ",
    "day ",
    "daybreak ",
    "deal ",
    "duck ",
    "ducked ",
    "ducks ",
    "death ",
    "deathes ",
    "debtor ",
    "debtors ",
    "deeply ",
    "defender ",
    "defends ",
    "defect ",
    "defective ",
    "defends ",
    "dollars ",
    "donation ",
    "download ",
    "downfall ",
    "dragon ",
    "dragons ",
    "draw ",
    "drank ",
    "eat ",
    "eaten ",
    "east ",
    "eastily ",
    "echo ",
    "edit ",
    "earth ",
    "ebb ",
    "edition ",
    "educate ",
    "effect ",
    "effected ",
    "elevator ",
    "elephant ",
    "elevation ",
    "else ",
    "elsewhere ",
    "email ",
    "embryo ",
    "emotions ",
    "emotion ",
    "end ",
    "enemy ",
    "enemies ",
    "engine ",
    "enter ",
    "entered ",
    "enters ",
    "envious ",
    "events ",
    "event ",
    "false ",
    "fair ",
    "fame ",
    "familiar ",
    "family ",
    "famine ",
    "famous ",
    "fancy ",
    "fan ",
    "fans ",
    "fanat ",
    "fanats ",
    "fanned ",
    "fashion ",
    "fared ",
    "fast ",
    "faster ",
    "few ",
    "figure ",
    "fight ",
    "fights ",
    "fifty ",
    "find ",
    "fire ",
    "fix ",
    "fixed ",
    "food ",
    "folk ",
    "force ",
    "follow ",
    "followers ",
    "follows ",
    "forget ",
    "forging ",
    "forgot ",
    "forms ",
    "forsaken ", //bad game
    "free ",
    "frame ",
    "freely ",
    "freedom ",
    "freeman ",
    "freeze ",
    "fresh ",
    "girl ",
    "girls ",
    "genius ",
    "give ",
    "given ",
    "glad ",
    "gladly ",
    "glass ",
    "gleam ",
    "get ",
    "getting ",
    "general ",
    "generally ",
    "gen ",
    "girded ",
    "glass ",
    "glow ",
    "go ",
    "goat ",
    "god ",
    "goal ",
    "grow ",
    "growed up ",
    "grown ",
    "guard ",
    "guarding ship ",
    "health ",
    "healhy ",
    "head ",
    "heaps ",
    "hear ",
    "hate ",
    "hated ",
    "here ",
    "high ",
    "higher ",
    "highest ",
    "heal ",
    "hint ",
    "himself ",
    "horse ",
    "horror ",
    "holly ",
    "images ",
    "idle ",
    "imagination ",
    "imaginary ",
    "imitate ",
    "imitated ",
    "include ",
    "incorrect ",
    "incorruption",
    "india ",
    "italy ",
    "jew ",
    "jessus ",
    "join ",
    "joke ",
    "jupiter ",
    "juno ",
    "junior ",
    "judge ",
    "keeps ",
    "knock ",
    "know ",
    "knowest ",
    "knowledge ",
    "language ",
    "lament ",
    "larp ",
    "larped ",
    "larping ",
    "languages ",
    "land ",
    "lands ",
    "large ",
    "largeness ",
    "live ",
    "likes ",
    "like ",
    "literary ",
    "load ",
    "loads ",
    "lively ",
    "main ",
    // so bores, i gonna just add random words
    /* ==random words==*/
    "loader ",
    "miner ",
    "Pro ",
    "noob ",
    "good ",
    "bad ",
    "word ",
    "words ",
    "ukraine ",
    "ukrainians ",
    "italy ",
    "italians ",
    "japan ",
    "japanese ",
    "germany ",
    "german ",
    "british ",
    "britania ",
    "united kingdom ",
    "speed ",
    "ishowspeed ",
    "berlin ",
    "Denmark ",
    "Moldavia ",
    "Moldovian ",
    "USA ",
    "America ",
    "Americano ",
    "coffee ",
    "speeds ",
    "many ",
    "width ",
    "height ",
    "end ",
    "of world",
    // add later more
};


/* =========================================================
   Word count
   ========================================================= */

#define WORD_COUNT \
    (sizeof(words) / sizeof(words[0]))


/* =========================================================
   Random sentence
   ========================================================= */

void say(void)
{
    print("\n\n");

    print("Program says: ");

    for (int i = 0; i < 17; i++)
    {
        int randomword;

        randomword =
            random_range(0, WORD_COUNT - 1);

        print(words[randomword]);
    }

    print("\n\n");
}


/* =========================================================
   Computer chat
   ========================================================= */

void chat(void)
{
    char input[128];

    print("\n");
    print("================================\n");
    print("       NwOS COMPUTER CHAT\n");
    print("================================\n");

    print("Type !0 to leave the chat.\n\n");


    while (1)
    {
        print("You: ");

        read_line(
            input,
            128
        );


        /* -----------------------------------------
           Exit
           ----------------------------------------- */

        if (strcmp(input, "!0") == 0)
        {
            print("\n");
            print("Computer: Goodbye!\n");
            print("Chat closed.\n\n");

            return;
        }


        /* -----------------------------------------
           Input does not affect the answer.
           Generate random words.
           ----------------------------------------- */

        print("\n");
        print("Computer: ");

        for (int i = 0; i < 17; i++)
        {
            int randomword;

            randomword =
                random_range(0, WORD_COUNT - 1);

            print(words[randomword]);
        }

        print("\n\n");
    }
}