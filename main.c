//#include "outermeSDL.h"
#include "SDLGateway.h"

#define checkSKUp keyStates[SC_UP]
#define checkSKDown keyStates[SC_DOWN]
#define checkSKLeft keyStates[SC_LEFT]
#define checkSKRight keyStates[SC_RIGHT]
#define checkSKAttack keyStates[SC_ATTACK]
#define checkSKInteract keyStates[SC_INTERACT]
#define checkSKMenu keyStates[SC_MENU]
#define TILE_ID_PLAYER 16
#define PIXELS_MOVED 6

#define WINDOW_NAME "Gateway to Legend"
#define CONFIG_FILEPATH "GatewayToLegend.cfg"
#define GLOBALTILES_FILEPATH "tileset/mainTileset8x6.png"
#define GLOBALSAVE_FILEPATH "saves/GATEWAY_MAIN.txt"
#define MAP_PACKS_SUBFOLDER "map-packs/"
#define MAX_LIST_OF_MAPS 30
#define MAX_CHAR_IN_FILEPATH MAX_PATH

#define MAX_MAPPACKS_PER_PAGE 11

#define START_GAMECODE 0
#define OPTIONS_GAMECODE 1
#define PLAY_GAMECODE 2
#define MAINLOOP_GAMECODE 3
#define OVERWORLDMENU_GAMECODE 4
#define RELOAD_GAMECODE 5
#define SAVE_GAMECODE 6

#define MAX_TILE_ID_ARRAY 17
#define MAX_COLLISIONDATA_ARRAY 10

#define drawASprite(tileset, spr) drawATile(tileset, spr.tileIndex, spr.x, spr.y, spr.w, spr.h, spr.angle, spr.flip)

int mainLoop(player* playerSprite);
void checkCollision(player* player, int* outputData, int moveX, int moveY);
void mapSelectLoop(char** listOfFilenames, char* mapPackName, int maxStrNum, bool* backFlag);
void drawOverTilemap(SDL_Texture* texture, int startX, int startY, int endX, int endY, bool drawDoors[], bool rerender);

/*bool debug;
bool doDebugDraw;
SDL_Texture* eventTexture;  //eventmap layer is needed, this is just for debug, so when you're all done you can prob remove these*/

int tileIDArray[MAX_TILE_ID_ARRAY];
#define PLAYER_ID tileIDArray[0]
#define CURSOR_ID tileIDArray[1]
#define HP_ID tileIDArray[2]
#define SWORD_ID tileIDArray[13]
#define ENEMY(x) tileIDArray[13 + x]

bool doorFlags[3] = {true, true, true};  //this works; however it persists through map packs as well
script* allScripts;
int sizeOfAllScripts;

int main(int argc, char* argv[])
{
    //setting up default values
    //debug = true;
    {
        int initCode = initSDL(WINDOW_NAME, GLOBALTILES_FILEPATH, FONT_FILE_NAME, SCREEN_WIDTH, SCREEN_HEIGHT, 48);
        if (initCode != 0)
            return initCode;
    }
    //loading in map pack header files
    char mainFilePath[MAX_CHAR_IN_FILEPATH], mapFilePath[MAX_CHAR_IN_FILEPATH - 9], tileFilePath[MAX_CHAR_IN_FILEPATH - 9],
            saveFilePath[MAX_CHAR_IN_FILEPATH - 9], scriptFilePath[MAX_CHAR_IN_FILEPATH - 9];
    char** listOfFilenames;
    int maxStrNum = 0;
    listOfFilenames = getListOfFiles(MAX_LIST_OF_MAPS, MAX_CHAR_IN_FILEPATH - 9, MAP_PACKS_SUBFOLDER, &maxStrNum);
    //done loading map pack header files
    player person;
    if (checkFile(GLOBALSAVE_FILEPATH, 0))
        loadGlobalPlayer(&person, GLOBALSAVE_FILEPATH);
    else
        createGlobalPlayer(&person, GLOBALSAVE_FILEPATH);
    if (checkFile(CONFIG_FILEPATH, 6))  //load config
        loadConfig(CONFIG_FILEPATH);
    else
        initConfig(CONFIG_FILEPATH);
    /*if (debug)
        loadIMG("tileset/eventTile48.png", &eventTexture);*/
    SDL_SetRenderDrawBlendMode(mainRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(mainRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(mainRenderer);
    int gameState = 0;
    char* buffer = "";  //actually needed
    bool quitGame = false;
    allScripts = NULL;
    int choice = 0;
    while(!quitGame)
    {
        switch(gameState)
        {
        case START_GAMECODE:  //start menu
            person.mapScreen = 0;
            choice = aMenu(tilesetTexture, 17, "Gateway to Legend", "Play", "Options", "Quit", " ", "(Not final menu)", 3, 1, (SDL_Color) {0xFF, 0xFF, 0xFF, 0xFF}, (SDL_Color) {0xA5, 0xA5, 0xA5, 0xFF}, (SDL_Color) {0x00, 0x00, 0x00, 0xFF}, (SDL_Color) {0x00, 0x00, 0x00, 0xFF}, true, true);
            if (choice == 1)
                gameState = PLAY_GAMECODE;
            if (choice == 2)
                gameState = OPTIONS_GAMECODE;
            if (choice == 3)
                quitGame = true;
            break;
        case OPTIONS_GAMECODE:
            gameState = START_GAMECODE;
            break;
        case PLAY_GAMECODE:  //main menu
            mapSelectLoop(listOfFilenames, (char*) mainFilePath, maxStrNum, &quitGame);
            if (quitGame)  //yes I do need this, this is gonna tell me if we
            {
                gameState = START_GAMECODE;
                quitGame = false;
                break;
            }
            //loading map pack stuff
            uniqueReadLine((char**) &mapFilePath, MAX_CHAR_IN_FILEPATH - 9, mainFilePath, 1);
            //printf("%s\n", mapFilePath);
            uniqueReadLine((char**) &tileFilePath, MAX_CHAR_IN_FILEPATH - 9, mainFilePath, 2);
            //printf("%s\n", tileFilePath);
            uniqueReadLine((char**) &saveFilePath, MAX_CHAR_IN_FILEPATH - 9, mainFilePath, 3);
            //printf("%s\n", saveFilePath);
            uniqueReadLine((char**) &scriptFilePath, MAX_CHAR_IN_FILEPATH - 9, mainFilePath, 4);
            //printf("%s\n", scriptFilePath);
            loadIMG(tileFilePath, &tilesTexture);
            free(allScripts);
            allScripts = calloc(checkFile(scriptFilePath, -1) + 1, sizeof(script));
            for(int i = 0; i < checkFile(scriptFilePath, -1) + 1; i++)
            {
                script thisScript;
                readScript(&thisScript, readLine(scriptFilePath, i, &buffer));
                allScripts[i] = thisScript;
                sizeOfAllScripts = i + 1;
            }
            for(int i = 0; i < MAX_TILE_ID_ARRAY; i++)
            {
                tileIDArray[i] = strtol(readLine(mainFilePath, 8 + i, &buffer), NULL, 10);
            }
            quitGame = aMenu(tilesTexture, CURSOR_ID, readLine(mainFilePath, 0, &buffer), "New Game", "Load Game", "Back", " ", " ", 3, 2, (SDL_Color) {0xFF, 0xFF, 0xFF, 0xFF}, (SDL_Color) {0xA5, 0xA5, 0xA5, 0xFF}, (SDL_Color) {0x00, 0x00, 0x00, 0xFF}, (SDL_Color) {0x00, 0x00, 0x00, 0xFF}, true, false);
            if (quitGame == 3)
            {
                quitGame = false;
                break;
            }
            if (checkFile(saveFilePath, 0) && quitGame == 2)
                loadLocalPlayer(&person, saveFilePath, PLAYER_ID);
            else
                createLocalPlayer(&person, saveFilePath, strtol(readLine(mainFilePath, 5, &buffer), NULL, 10), strtol(readLine(mainFilePath, 6, &buffer), NULL, 10), TILE_SIZE, person.mapScreen, strtol(readLine(mainFilePath, 7, &buffer), NULL, 10), SDL_FLIP_NONE, PLAYER_ID);
            quitGame = false;
            //done loading map-pack specific stuff
            if (checkFile(GLOBALSAVE_FILEPATH, 0))
                loadGlobalPlayer(&person, GLOBALSAVE_FILEPATH);  //loaded twice just to ensure nothing is overwritten
            else
                createGlobalPlayer(&person, GLOBALSAVE_FILEPATH);
            gameState = MAINLOOP_GAMECODE;
            break;
        case MAINLOOP_GAMECODE:  //main game loop
            loadMapFile(mapFilePath, tilemap, eventmap, person.mapScreen, HEIGHT_IN_TILES, WIDTH_IN_TILES);
            person.extraData = mapFilePath;
            choice = mainLoop(&person);
            if (choice == ANYWHERE_QUIT)
                quitGame = true;
            if (choice == 1)
                gameState = OVERWORLDMENU_GAMECODE;
            if (choice == 2)
                gameState = RELOAD_GAMECODE;
            break;
        case OVERWORLDMENU_GAMECODE:  //overworld menu
            choice = aMenu(tilesTexture, CURSOR_ID, "Overworld Menu", "Back", "Save", "Exit", " ", " " , 3, 1, (SDL_Color) {0xFF, 0xFF, 0xFF, 0xFF}, (SDL_Color) {0xA5, 0xA5, 0xA5, 0xFF}, (SDL_Color) {0x00, 0x00, 0x00, 0xFF}, (SDL_Color) {0x00, 0x00, 0x00, 0xFF}, true, false);
            if (choice == 1)
                gameState = MAINLOOP_GAMECODE;
            if (choice == 2 || choice == 3)
                gameState = SAVE_GAMECODE;
            if (choice == 3)
            {
                for(int i = 0; i < 3; i++)
                    doorFlags[i] = true;
            }
            break;
        case RELOAD_GAMECODE:
            gameState = MAINLOOP_GAMECODE;
            for(int i = 0; i < 3; i++)
                doorFlags[i] = true;
            break;
        case SAVE_GAMECODE:
            saveLocalPlayer(person, saveFilePath);
            if (choice == 2)
                gameState = MAINLOOP_GAMECODE;
            if (choice == 3)
                gameState = START_GAMECODE;
            break;
        }
    }
    saveGlobalPlayer(person, GLOBALSAVE_FILEPATH);
    //SDL_DestroyTexture(eventTexture);  //once we delete eventTexture, you can remove this.
    SDL_DestroyTexture(tilesTexture);
    free(allScripts);
    closeSDL();
    return 0;
}

void mapSelectLoop(char** listOfFilenames, char* mapPackName, int maxStrNum, bool* backFlag)
{
    bool quitMenu = false;
    char junkArray[MAX_CHAR_IN_FILEPATH];
    SDL_Keycode menuKeycode;
    int menuPage = 0, selectItem = 0;
    while(!quitMenu)
    {
        SDL_SetRenderDrawColor(mainRenderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(mainRenderer);
        SDL_SetRenderDrawColor(mainRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderFillRect(mainRenderer, &((SDL_Rect){.x = SCREEN_WIDTH / 128, .y = SCREEN_HEIGHT / 128, .w = 126 * SCREEN_WIDTH / 128, .h = 126 * SCREEN_HEIGHT / 128}));
        for(int i = 0; i < (maxStrNum - menuPage * MAX_MAPPACKS_PER_PAGE > MAX_MAPPACKS_PER_PAGE ? MAX_MAPPACKS_PER_PAGE : maxStrNum - menuPage * MAX_MAPPACKS_PER_PAGE); i++)  //11 can comfortably be max
            drawText(readLine((char*) strcat(strcpy(junkArray, MAP_PACKS_SUBFOLDER), listOfFilenames[i + (menuPage * 5)]),  /*concatting the path and one of the filenames together into one string*/
                          0, (char**) &junkArray), TILE_SIZE + 10, (i + 3) * TILE_SIZE, SCREEN_WIDTH, SCREEN_HEIGHT, (SDL_Color) {0, 0, 0}, false);
        drawText("Back", TILE_SIZE + 10, 2 * TILE_SIZE, SCREEN_WIDTH, TILE_SIZE, (SDL_Color) {0, 0, 0}, false);
        menuKeycode = getKey();
        if ((menuKeycode == SDL_GetKeyFromScancode(SC_LEFT) && menuPage > 0) || (menuKeycode == SDL_GetKeyFromScancode(SC_RIGHT) && menuPage < maxStrNum / MAX_MAPPACKS_PER_PAGE))
        {
            menuPage += (menuKeycode == SDL_GetKeyFromScancode(SC_RIGHT)) - 1 * (menuKeycode == SDL_GetKeyFromScancode(SC_LEFT));
            selectItem = 0;
        }

        if ((menuKeycode == SDL_GetKeyFromScancode(SC_UP) && selectItem > 0) || (menuKeycode == SDL_GetKeyFromScancode(SC_DOWN) && selectItem < (maxStrNum - menuPage * MAX_MAPPACKS_PER_PAGE > MAX_MAPPACKS_PER_PAGE ? MAX_MAPPACKS_PER_PAGE : maxStrNum - menuPage * MAX_MAPPACKS_PER_PAGE)))
            selectItem += (menuKeycode == SDL_GetKeyFromScancode(SC_DOWN)) - 1 * (menuKeycode == SDL_GetKeyFromScancode(SC_UP));

        drawTile(17, 10, (selectItem + 2) * TILE_SIZE, TILE_SIZE, 0, SDL_FLIP_NONE);
        SDL_RenderPresent(mainRenderer);

        if (menuKeycode == SDL_GetKeyFromScancode(SC_INTERACT))
        {
            if (selectItem != 0)
                selectItem = menuPage * MAX_MAPPACKS_PER_PAGE + selectItem - 1;
            else
                *backFlag = true;
                quitMenu = true;
        }
    }
    //loading map pack stuff
    strncat(strcpy(mapPackName, MAP_PACKS_SUBFOLDER), listOfFilenames[selectItem], MAX_CHAR_IN_FILEPATH - 9);
}

int mainLoop(player* playerSprite)
{
    SDL_Event e;
    bool quit = false;
    static bool textBoxOn = false;
    char mapFilePath[MAX_CHAR_IN_FILEPATH];
    strcpy(mapFilePath, playerSprite->extraData);
    int maxTheseScripts = 0, * collisionData = calloc(MAX_COLLISIONDATA_ARRAY, sizeof(int));
    script thisScript, * theseScripts = calloc(sizeOfAllScripts, sizeof(script));
    thisScript.active = false;
    for(int i = 0; i < sizeOfAllScripts; i++)
    {
        if (allScripts[i].mapNum == playerSprite->mapScreen)
            theseScripts[maxTheseScripts++] = allScripts[i];
    }
    {
        script* new_ptr = realloc(theseScripts, maxTheseScripts * sizeof(script));
        if (new_ptr != NULL)
            theseScripts = new_ptr;
    }
    sprite enemies[6];
    int enemyCount = 0;
    for(int y = 0; y < HEIGHT_IN_TILES; y++)
    {
        for(int x = 0; x < WIDTH_IN_TILES; x++)
        {
            if(eventmap[y][x] == 12 && enemyCount < 6)
                initSprite(&enemies[enemyCount++], x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, ENEMY(1), 0, SDL_FLIP_NONE, type_enemy);

            if(eventmap[y][x] == 13 && enemyCount < 6)
                initSprite(&enemies[enemyCount++], x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, ENEMY(2), 0, SDL_FLIP_NONE, type_enemy);

            if(eventmap[y][x] == 14 && enemyCount < 6)
                initSprite(&enemies[enemyCount++], x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, ENEMY(3), 0, SDL_FLIP_NONE, type_enemy);
        }
    }
    //printf("%d < %d\n", maxTheseScripts, sizeOfAllScripts);
    //doDebugDraw = false;
    int exitCode = 2;
    char whatever[5] = "    \0";
    int startTime = SDL_GetTicks(), lastFrame = startTime,
        frame = 0, framerate = 0, sleepFor = 0, lastUpdateTime = SDL_GetTicks(),
        swordTimer = 0;
    sprite sword;
    initSprite(&sword, 0, 0, TILE_SIZE, SWORD_ID, 0, SDL_FLIP_NONE, type_na);
    while(!quit && playerSprite->HP > 0)
    {
        SDL_RenderClear(mainRenderer);
        drawATilemap(tilesTexture, tilemap, 0, 0, 20, 15, -1, false);
        drawOverTilemap(tilesTexture, 0, 0, 20, 15, doorFlags, false);
        {  //drawing HUD
            SDL_SetRenderDrawColor(mainRenderer, 0, 0, 0x21, 0x7F);
            SDL_RenderFillRect(mainRenderer, &((SDL_Rect) {.x = 0, .y = 0, .w = playerSprite->maxHP / 4 * TILE_SIZE, .h = TILE_SIZE}));
            for(int i = 0; i < playerSprite->HP; i += 4)  //draw HP
                drawATile(tilesTexture, HP_ID, TILE_SIZE * (i / 4), 0, (playerSprite->HP - i - 4 > 0 ? 4 : playerSprite->HP - i - 4 % 4) * (TILE_SIZE / 4), TILE_SIZE, 0, SDL_FLIP_NONE);
        }
        /*if (doDebugDraw)
            drawATilemap(eventTexture, true, 0, 0, 20, 15, false);*/
        //drawTile(tilemap[playerSprite->spr.y / TILE_SIZE][playerSprite->spr.x / TILE_SIZE + 1 * (playerSprite->spr.x % TILE_SIZE > .5 * TILE_SIZE)], (playerSprite->spr.x / TILE_SIZE  + 1 * (playerSprite->spr.x % TILE_SIZE > .5 * TILE_SIZE)) * TILE_SIZE, (playerSprite->spr.y / TILE_SIZE) * TILE_SIZE, TILE_SIZE, SDL_FLIP_NONE);
        while(SDL_PollEvent(&e) != 0)  //while there are events in the queue
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
                exitCode = ANYWHERE_QUIT;
            }
            /*if (e.key.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_g && debug)
                doDebugDraw = !doDebugDraw;*/
        }
        const Uint8* keyStates = SDL_GetKeyboardState(NULL);
        if (!checkSKAttack)
                textBoxOn = false;

        if (SDL_GetTicks() - lastUpdateTime >= 32)
        {
            if (!playerSprite->movementLocked && (checkSKUp || checkSKDown || checkSKLeft || checkSKRight || checkSKAttack || checkSKInteract || playerSprite->xVeloc || playerSprite->yVeloc))
            {
                int lastY = playerSprite->spr.y;
                int lastX = playerSprite->spr.x;

                if (playerSprite->spr.y > 0 && checkSKUp)
                    playerSprite->spr.y -= PIXELS_MOVED;

                if (playerSprite->spr.y < SCREEN_HEIGHT - playerSprite->spr.h && checkSKDown)
                    playerSprite->spr.y += PIXELS_MOVED;

                if (playerSprite->spr.x > 0 && checkSKLeft)
                    playerSprite->spr.x -= PIXELS_MOVED;

                if (playerSprite->spr.x < SCREEN_WIDTH - playerSprite->spr.w && checkSKRight)
                    playerSprite->spr.x += PIXELS_MOVED;

                if (checkSKAttack && !textBoxOn && frame > targetTime / 2)
                {
                    initScript(&thisScript, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, script_trigger_dialogue, "Hello world!");
                    textBoxOn = true;
                }

                if ((lastX != playerSprite->spr.x || lastY != playerSprite->spr.y) && !checkSKAttack)
                    playerSprite->lastDirection = checkSKUp + 2 * checkSKDown + 4 * checkSKLeft + 8 * checkSKRight;

                if (playerSprite->lastDirection / 4 == 1)
                    playerSprite->spr.flip = SDL_FLIP_HORIZONTAL;
                else
                    playerSprite->spr.flip = SDL_FLIP_NONE;

                if (playerSprite->xVeloc)
                    playerSprite->spr.x += playerSprite->xVeloc;

                if (playerSprite->yVeloc)
                    playerSprite->spr.y += playerSprite->yVeloc;

                checkCollision(playerSprite, collisionData, (checkSKRight || playerSprite->xVeloc > 0) + -1 * (checkSKLeft || playerSprite->xVeloc < 0), (checkSKDown || playerSprite->yVeloc > 0) + -1 * (checkSKUp || playerSprite->yVeloc < 0));

                if (checkSKInteract || swordTimer)
                {
                    int xDir = (playerSprite->lastDirection / 4) % 3;  //mod 3 to get rid of a value of 3 -- 3 == both directions pressed, or 0 movement
                    int yDir = (playerSprite->lastDirection - xDir * 4) % 3 - 1;  //subtract 1 to turn either 0, 1, or 2 into either -1, 0, or 1
                    if ((xDir -= 1) != -1)
                        xDir -= !xDir;  //turns 0 and 1 into -1 and 1
                    else
                        xDir = 0;

                    if (yDir != -1)
                        yDir -= !yDir;
                    else
                        yDir = 0;
                    yDir *= !xDir;  //x direction takes precedence over y direction
                    initSprite(&sword, playerSprite->spr.x + TILE_SIZE * xDir, playerSprite->spr.y + TILE_SIZE * yDir, TILE_SIZE, SWORD_ID, 90 * yDir, SDL_FLIP_HORIZONTAL * (xDir == -1), type_na);

                    if (!swordTimer)
                        swordTimer = SDL_GetTicks() + 750;
                }

                if (playerSprite->xVeloc)  //this is done so that the last frame of velocity input is still collision-checked
                    playerSprite->xVeloc -= 6 - 12 * (playerSprite->xVeloc < 0);

                if (playerSprite->yVeloc)
                    playerSprite->yVeloc -= 6 - 12 * (playerSprite->yVeloc < 0);

                if (!playerSprite->spr.x || !playerSprite->spr.y || playerSprite->spr.x == SCREEN_WIDTH - TILE_SIZE || playerSprite->spr.y == SCREEN_HEIGHT - TILE_SIZE)
                {
                    bool quitThis = false;
                    if (!playerSprite->spr.x && playerSprite->mapScreen % 10 > 0)
                    {
                        playerSprite->spr.x = SCREEN_WIDTH - (2 * TILE_SIZE);
                        playerSprite->mapScreen--;
                        quitThis = true;
                    }

                    if (!playerSprite->spr.y && playerSprite->mapScreen / 10 > 0)
                    {
                        playerSprite->spr.y = SCREEN_HEIGHT - (2 * TILE_SIZE);
                        playerSprite->mapScreen -= 10;
                        quitThis = true;
                    }

                    if (playerSprite->spr.x == SCREEN_WIDTH - TILE_SIZE && playerSprite->mapScreen % 10 < 9)
                    {
                        playerSprite->spr.x = TILE_SIZE;
                        playerSprite->mapScreen++;
                        quitThis = true;
                    }

                    if (playerSprite->spr.y == SCREEN_HEIGHT - TILE_SIZE && playerSprite->mapScreen / 10 < 9)
                    {
                        playerSprite->spr.y = TILE_SIZE;
                        playerSprite->mapScreen += 10;
                        quitThis = true;
                    }

                    if (quitThis)
                    {
                        quit = true;
                        exitCode = 2;
                    }
                }

                if (collisionData[0] || ((collisionData[4] && doorFlags[0] == true) || (collisionData[5] && doorFlags[1] == true) || (collisionData[6] && doorFlags[2] == true)))
                {
                    playerSprite->spr.y = lastY;
                    playerSprite->spr.x = lastX;
                    //printf("%d\n", exitCode);
                }
                if (collisionData[1] || collisionData[2] || collisionData[3])
                {
                    for(int i = 0; i < 3; i++)
                    {
                        if (collisionData[i + 1])
                            doorFlags[i] = false;
                    }
                }
                if (collisionData[8] && !playerSprite->invincCounter)
                {
                    playerSprite->xVeloc -= 24 * (checkSKRight + -1 * checkSKLeft);
                    playerSprite->yVeloc -= 24 * (checkSKDown + -1 * checkSKUp);
                    playerSprite->HP -= 1;
                    playerSprite->invincCounter = 24;
                }
                if (collisionData[9])
                {
                    bool found = false;
                    for(int i = 0; i < maxTheseScripts; i++)
                    {
                        if (theseScripts[i].action == script_use_warp_gate && SDL_HasIntersection(&((SDL_Rect){.x = playerSprite->spr.x, .y = playerSprite->spr.y, .w = playerSprite->spr.w, .h = playerSprite->spr.h}), &((SDL_Rect){.x = theseScripts[i].x, .y = theseScripts[i].y, .w = theseScripts[i].w, .h = theseScripts[i].h})))
                            {
                                thisScript = theseScripts[i];
                                found = true;
                                break;
                            }
                    }
                    thisScript.active = found;
                    //initScript(&thisScript, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, script_use_portal, "[0/456/336]\0");
                    playerSprite->extraData = mapFilePath;
                    exitCode = 2;
                }
            }
            for(int i = 0; i < enemyCount; i++)
            {
                if (enemies[i].tileIndex == ENEMY(1) && enemies[i].type == type_enemy)
                {
                    //behavior: move quickly at player, with little HP
                    if (SDL_HasIntersection(&(SDL_Rect) {.x = sword.x, .y = sword.y, .w = sword.w, .h = sword.h},
                                            &(SDL_Rect) {.x = enemies[i].x, .y = enemies[i].y, .w = enemies[i].w, .h = enemies[i].h})
                        && swordTimer > SDL_GetTicks() + 250)
                        enemies[i].type = type_na;

                    if (SDL_HasIntersection(&(SDL_Rect) {.x = playerSprite->spr.x, .y = playerSprite->spr.y, .w = playerSprite->spr.w,
                                            .h = playerSprite->spr.h}, &(SDL_Rect) {.x = enemies[i].x, .y = enemies[i].y, .w = enemies[i].w,
                                            .h = enemies[i].h}) && !playerSprite->invincCounter)
                    {
                         playerSprite->HP--;
                         playerSprite->xVeloc += 24 * (abs(playerSprite->spr.x - enemies[i].x) > abs(playerSprite->spr.y - enemies[i].y))
                         - 48 * (enemies[i].x > playerSprite->spr.x);

                         playerSprite->yVeloc += 24 * (abs(playerSprite->spr.y - enemies[i].y) > abs(playerSprite->spr.x - enemies[i].x))
                         - 48 * (enemies[i].y > playerSprite->spr.y);
                         playerSprite->invincCounter = 10;
                    }

                    if (enemies[i].x != playerSprite->spr.x)
                        enemies[i].x += 3 - 6 * (playerSprite->spr.x < enemies[i].x);
                    if (enemies[i].y != playerSprite->spr.y)
                        enemies[i].y += 3 - 6 * (playerSprite->spr.y < enemies[i].y);

                    //todo:check collision with environment
                }

                if (enemies[i].tileIndex == ENEMY(2) && enemies[i].type == type_enemy)
                {
                    //behavior: burst movement towards player?
                    if (SDL_HasIntersection(&(SDL_Rect) {.x = sword.x, .y = sword.y, .w = sword.w, .h = sword.h},
                                            &(SDL_Rect) {.x = enemies[i].x, .y = enemies[i].y, .w = enemies[i].w, .h = enemies[i].h})
                        && swordTimer > SDL_GetTicks() + 250)
                        enemies[i].type = type_na;

                    if (SDL_HasIntersection(&(SDL_Rect) {.x = playerSprite->spr.x, .y = playerSprite->spr.y, .w = playerSprite->spr.w,
                                            .h = playerSprite->spr.h}, &(SDL_Rect) {.x = enemies[i].x, .y = enemies[i].y, .w = enemies[i].w,
                                            .h = enemies[i].h}) && !playerSprite->invincCounter)
                    {
                         playerSprite->HP--;
                         playerSprite->xVeloc += 24 * (abs(playerSprite->spr.x - enemies[i].x) > abs(playerSprite->spr.y - enemies[i].y))
                         - 48 * (enemies[i].x > playerSprite->spr.x);

                         playerSprite->yVeloc += 24 * (abs(playerSprite->spr.y - enemies[i].y) > abs(playerSprite->spr.x - enemies[i].x))
                         - 48 * (enemies[i].y > playerSprite->spr.y);
                         playerSprite->invincCounter = 10;
                    }
                    //todo: move. no collision
                }

                if (enemies[i].tileIndex == ENEMY(3) && enemies[i].type == type_enemy)
                {
                    //behavior: move slowly at player, matching up x coord first then y, w/ lot of HP
                    if (SDL_HasIntersection(&(SDL_Rect) {.x = sword.x, .y = sword.y, .w = sword.w, .h = sword.h},
                                            &(SDL_Rect) {.x = enemies[i].x, .y = enemies[i].y, .w = enemies[i].w, .h = enemies[i].h})
                        && swordTimer > SDL_GetTicks() + 250)
                        enemies[i].type = type_na;

                    if (SDL_HasIntersection(&(SDL_Rect) {.x = playerSprite->spr.x, .y = playerSprite->spr.y, .w = playerSprite->spr.w,
                                            .h = playerSprite->spr.h}, &(SDL_Rect) {.x = enemies[i].x, .y = enemies[i].y, .w = enemies[i].w,
                                            .h = enemies[i].h}) && !playerSprite->invincCounter)
                    {
                         playerSprite->HP -= 2;
                         playerSprite->xVeloc += 24 * (abs(playerSprite->spr.x - enemies[i].x) > abs(playerSprite->spr.y - enemies[i].y))
                         - 48 * (enemies[i].x > playerSprite->spr.x);

                         playerSprite->yVeloc += 24 * (abs(playerSprite->spr.y - enemies[i].y) > abs(playerSprite->spr.x - enemies[i].x))
                         - 48 * (enemies[i].y > playerSprite->spr.y);
                         playerSprite->invincCounter = 10;
                    }

                    if (enemies[i].x != playerSprite->spr.x)
                        enemies[i].x += 2 - 4 * (playerSprite->spr.x < enemies[i].x);
                    if (enemies[i].y != playerSprite->spr.y && enemies[i].x == playerSprite->spr.x)
                        enemies[i].y += 2 - 4 * (playerSprite->spr.y < enemies[i].y);
                    //todo: check collision with environment
                }
            }
            if (playerSprite->invincCounter)
                playerSprite->invincCounter--;
            lastUpdateTime = SDL_GetTicks();
        }
        if (checkSKMenu)
        {
            quit = true;
            exitCode = 1;
        }

        if (swordTimer && SDL_GetTicks() >= swordTimer)
            swordTimer = 0;

        frame++;
        //if ((SDL_GetTicks() - startTime) % 250 == 0)
            framerate = (int) (frame / ((SDL_GetTicks() - startTime) / 1000.0));
        //printf("%d / %f == %d\n", frame, (SDL_GetTicks() - startTime) / 1000.0, framerate);
        if(keyStates[SDL_SCANCODE_F12])
            drawText(intToString(framerate, whatever), 0, 0, SCREEN_WIDTH, TILE_SIZE, (SDL_Color){0xFF, 0xFF, 0xFF, 0xFF}, false);
        //printf("Framerate: %d\n", frame / ((int) now - (int) startTime));

        drawASprite(tilesTexture, playerSprite->spr);

        for(int i = 0; i < enemyCount; i++)
        {
            if (enemies[i].type == type_enemy)
                drawASprite(tilesTexture, enemies[i]);
        }

        if (swordTimer > SDL_GetTicks() + 250)
            drawASprite(tilesTexture, sword);
        SDL_RenderPresent(mainRenderer);
        if ((sleepFor = targetTime - (SDL_GetTicks() - lastFrame)) > 0)
            SDL_Delay(sleepFor);  //FPS limiter; rests for (16 - time spent) ms per frame, effectively making each frame run for ~16 ms, or 60 FPS
        lastFrame = SDL_GetTicks();
        if (thisScript.active)
            quit = executeScriptAction(&thisScript, playerSprite);
    }

    if (playerSprite->HP < 1)
    {
        exitCode = 1;
        playerSprite->HP = playerSprite->maxHP;
    }

    free(theseScripts);
    free(collisionData);
    return exitCode;
}

void checkCollision(player* player, int* outputData, int moveX, int moveY)
{
    for(int i = 0; i < MAX_COLLISIONDATA_ARRAY; i++)
    {
        outputData[i] = 0;
    }
    if (moveX || moveY)
    {
        int thisX = player->spr.x, thisY = player->spr.y;
        int topLeft = eventmap[thisY / TILE_SIZE][thisX / TILE_SIZE], topRight = eventmap[thisY / TILE_SIZE][thisX / TILE_SIZE + (thisX % TILE_SIZE != 0)], bottomLeft = eventmap[thisY / TILE_SIZE + (thisY % TILE_SIZE != 0)][thisX / TILE_SIZE], bottomRight = eventmap[thisY / TILE_SIZE + (thisY % TILE_SIZE != 0)][thisX / TILE_SIZE + (thisX % TILE_SIZE != 0)];
        if (-1 != checkArrayForIVal(1, (int[]) {topLeft, topRight, bottomLeft, bottomRight}, 4))
            outputData[0] = topLeft + 2 * topRight + 4 * bottomLeft + 8 * bottomRight;
        if (((outputData[0] == 1 || outputData[0] == 5) && moveX < 0 && moveY > 0) || ((outputData[0] == 2 || outputData[0] == 10) && moveX > 0 && moveY > 0) || ((outputData[0] == 4 || outputData[0] == 5) && moveX < 0 && moveY < 0) || ((outputData[0] == 8 || outputData[0] == 10) && moveX > 0 && moveY < 0))
        {  //manually adding y sliding
            outputData[0] = false;
            player->spr.x -= moveX * PIXELS_MOVED;
        }
        if (((outputData[0] == 1 || outputData[0] == 3) && moveX > 0 && moveY < 0) || ((outputData[0] == 2 || outputData[0] == 3) && moveX < 0 && moveY < 0) || ((outputData[0] == 4 || outputData[0] == 12) && moveX > 0 && moveY > 0) || ((outputData[0] == 8 || outputData[0] == 12) && moveX < 0 && moveY > 0))
        {  //manually adding x sliding
            outputData[0] = false;
            player->spr.y -= moveY * PIXELS_MOVED;
        }

        /*if (collideID && debug && doDebugDraw)
            printf("X - %d\n", collideID);*/
        /*if (debug && doDebugDraw)
        {
            drawTile(7, (thisX / TILE_SIZE) * TILE_SIZE, (thisY / TILE_SIZE) * TILE_SIZE, TILE_SIZE, SDL_FLIP_NONE);
            drawTile(15, (thisX / TILE_SIZE + (thisX % TILE_SIZE != 0)) * TILE_SIZE, (thisY / TILE_SIZE) * TILE_SIZE, TILE_SIZE, SDL_FLIP_NONE);
            drawTile(2, (thisX / TILE_SIZE) * TILE_SIZE, (thisY / TILE_SIZE + (thisY % TILE_SIZE != 0)) * TILE_SIZE, TILE_SIZE, SDL_FLIP_NONE);
            drawTile(9, (thisX / TILE_SIZE + (thisX % TILE_SIZE != 0)) * TILE_SIZE, (thisY / TILE_SIZE + (thisY % TILE_SIZE != 0)) * TILE_SIZE, TILE_SIZE, SDL_FLIP_NONE);
            SDL_RenderPresent(mainRenderer);
        }*/
        for(int i = 1; i < MAX_COLLISIONDATA_ARRAY; i++)
        {
            if (-1 != checkArrayForIVal(i + 1, (int[]) {topLeft, topRight, bottomLeft, bottomRight}, 4))
            outputData[i] = true;
        }
    }
}

void drawOverTilemap(SDL_Texture* texture, int startX, int startY, int endX, int endY, bool drawDoors[], bool rerender)
{
    int searchIndex = 0;
    for(int y = startY; y < endY; y++)
        for(int x = startX; x < endX; x++)
        {
            searchIndex = eventmap[y][x] + 3 - (eventmap[y][x] > 0);  //search index for these tiles is beyond HUD/player slots. Minus 1 because there's only 1 index for invis tile but two cases right next to each other that need it
            if (((searchIndex == 7 || searchIndex == 8 || searchIndex == 9) && drawDoors[searchIndex < 10 ? searchIndex - 7 : 0] == false) || (searchIndex == 14 || searchIndex == 15 || searchIndex == 16))  //7,8,9 are the door indexes
                searchIndex = 3;  //3 is index for invis tile
            drawATile(texture, tileIDArray[searchIndex], x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, 0, SDL_FLIP_NONE);
        }
    if (rerender)
        SDL_RenderPresent(mainRenderer);
}
