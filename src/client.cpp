#include<iostream>
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_net.h>

struct player_t
{
	point_t pos;
	SDL_Rect screenPos;
	SDL_Rect frame;
	humanTemplate_t *body;
	char drawDirection = 0;

	player_t( int templateID, int x, int y )
	{
		body = &humanTemplates[ templateID ];
		frame.w = body->frame.w;
		frame.h = body->frame.h;
		//REMOVE
		frame.x = 0;
		frame.y = 0;
		screenPos.w = frame.w;
		screenPos.h = frame.h;

		pos.Set( x, y );	
	}
	player_t()
	{}
};

std::vector< player_t > players;

SDL_Renderer* renderer;
SDL_Event e;

TCPsocket serverSocket;
SDLNet_SocketSet chkClient = SDLNet_AllocSocketSet( 1 );
bool ConnectToServer( const char* address = "localhost", Uint16 port = DEFAULT_PORT )
{
	IPaddress ip;
    std::cout<<"Connecting to: "<<address<<" on port "<<port<<std::endl;
    SDLNet_ResolveHost( &ip, address, port );	
    serverSocket = SDLNet_TCP_Open( &ip );
    if( !serverSocket )
	{
        std::cout<<"Failed to connect"<<std::endl;
		return false;
	}
    else
	{
        SDLNet_TCP_AddSocket( chkClient, serverSocket );
		return true;
	}
}

#include "init.h"
#include "input.h"

void LoadTextures( texture_t &txtr )
{
	txtr.texture = IMG_LoadTexture( renderer, txtr.filename );
//	delete txtr.binaryTexture;
}
void LoadLocalTextures( const char *list )
{
	std::ifstream file( list );
	while( !file.eof() )
	{
		char textureName[500];
		file>>textureName;
		texture_t newTexture( textureName, false );
		LoadTextures( newTexture );
		textures.push_back( newTexture );
	}
}

void Quit()
{
	clientIsOn = false;
	SDL_Quit();
	SDL_DestroyWindow( window );
	std::exit( 0 );
}

SDL_Rect backgroundPos;
SDL_Rect defBackgroundPos;
texture_t background;
float scale = 1;

int netIDTable[2000];
int roadblockIDTable[2000];
int humanIDTable[2000];
int textureIDTable[2000];

int playerID = -1;
int myMessage[5];
bool activeClient = false;

void GetMessage();
bool recievedInitalInfo = false;

void ClientMain( const char* address = "localhost", Uint16 port = DEFAULT_PORT, const char* textureList = "textures.list" )
{

	clientIsOn = true;
    SDL_Init( 0 );
    InitInput();
    InitWindow();
    if( ShouldITurnVSync() )
		renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	else
		renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );
	//CHANGE
	LoadLocalTextures( textureList );
	for( int i=0;i<textures.size();i++ )
	{
		SDL_RenderCopy( renderer, textures[i].texture, NULL, NULL );
		SDL_RenderPresent( renderer );
		SDL_Delay( 2000 );
		SDL_RenderClear( renderer );
	}
	Quit();
	return;

//	if( address == "localhost" )
//		while( !serverIsOn ){}
    bool connection = ConnectToServer( address, port );
	if( !connection )
	{
		Quit();
		return;
	}

	std::cout<<"Getting info from server"<<std::endl;
	int active = SDLNet_CheckSockets( chkClient, 10000 );
	if( active <= 0 )
	{
		std::cout<<"Server didn't respond"<<std::endl;
		Quit();
	}
// Determine what info we need from server
// If there are loaded templates then we don't need that info from server
	Uint8 reqHumanTpltMeta[2] = {MSG_META_REQ_HUMAN_TEMPLATES, 1};
	int reqHumanTpltMessage[ reqHumanTpltMeta[1] ];
	if( humanTemplates.size() > 0 )
		reqHumanTpltMessage[0] = false;
	else
		reqHumanTpltMessage[0] = true;

	SDLNet_TCP_Send( serverSocket, reqHumanTpltMeta, 2 ); 
	SDLNet_TCP_Send( serverSocket, reqHumanTpltMessage, reqHumanTpltMeta[1]*4 );
/*
	GetMessage();
	active = SDLNet_CheckSockets( chkClient, 0 );

	if( playerID == -1 )
	{
		std::cout<<"ID error"<<std::endl;
		Quit();
	}
	*/

	while( !recievedInitalInfo and active > 0 )
	{
		std::cout<<"Client"<<std::endl;
		GetMessage();
		active = SDLNet_CheckSockets( chkClient, 0 );
	}
	if( !recievedInitalInfo )
	{
		std::cout<<"Didn't recieved flag for end from server, exiting..."<<std::endl;
		Quit();
	}
// Load textures from the UI
	texture_t numbers( "Textures/numbers.png", false );
	LoadTextures( numbers );

	for( int i=0;i<textures.size();i++ )
		LoadTextures( textures[i] );

	
	long long inputT = SDL_GetTicks();
	long long infoT = SDL_GetTicks();
	long long fpsT = SDL_GetTicks();
	long long drawSpellT = SDL_GetTicks();
	long long checkNetT = SDL_GetTicks();
    long long sendNetT = SDL_GetTicks();

    int frames;
    std::vector<int> sframes;
	sframes.push_back(0);
	
	std::cout<<"Client: Entering main loop"<<std::endl;
    while( true )
    {
        while( SDL_PollEvent( &e ) )
        {
            if( e.type == SDL_QUIT )
            {
				std::cout<<"Closing Client"<<std::endl;
                Quit();
            }
        }
        if( SDL_GetTicks() - inputT >= 10 )
		{
			ScanKeyboard();
			AnalyzeInput( myMessage, activeClient );
			ResetKeyboard();

			inputT = SDL_GetTicks();
		}

        if( SDL_GetTicks() - sendNetT >= 10 )
		{
			if( activeClient )
			{
				Uint8 meta[2] = {MSG_META_INPUT, 3};
				SDLNet_TCP_Send( serverSocket, meta, 2 );
				SDLNet_TCP_Send( serverSocket, myMessage, meta[1]*4 );
				for( int i=0;i<2;i++ )
					myMessage[i] = 0;
				activeClient = false;
			}
			sendNetT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - checkNetT >= 0 )
		{
			bool serverSend = SDLNet_CheckSockets( chkClient, 0 );
			while( serverSend )
			{
				GetMessage();
				serverSend = SDLNet_CheckSockets( chkClient, 0 );
			}
			checkNetT = SDL_GetTicks();
		}

        if( SDL_GetTicks() - fpsT >= 1000 and ShouldIDisplayFPS() )
		{
			//std::cout<<"FPS: "<<frames<<std::endl;
			std::vector<int> bucket;
			sframes.clear();
			while( frames > 0 )
			{
				bucket.push_back( frames%10 );
				frames/=10;
			}
			for( int i=0;bucket.size()!=0;i++ )
			{
				sframes.push_back(bucket.back());
				bucket.pop_back();
			}
			frames = 0;
			fpsT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - infoT >= 1000 )
		{
			std::cout<<SDL_GetError()<<std::endl;
			infoT = SDL_GetTicks();
		}

		if( background.texture != NULL )
	    	SDL_RenderCopy( renderer, background.texture, NULL, &backgroundPos );
		/*
		for( int i = 0;i<roadblock.size();i++ )
		{
			roadblock[i].screenPos.x = humans[playerID].screenPos.x - humans[playerID].pos.x + roadblock[i].pos.x;// + (roadblock[i]. .w - roadblock[i].w)/2;
			roadblock[i].screenPos.y = humans[playerID].screenPos.y - humans[playerID].pos.y + roadblock[i].pos.y;// + (roadblock[i].pos.h - roadblock[i].h)/2;
			SDL_RenderCopy( renderer, textures[roadblock[i].textureID].texture, &roadblock[i].frame, &roadblock[i].screenPos );
		}
		
        for( int i = 0;i<players.size();i++ )
		{
			int id = humanIDTable[i];
			if( id!=playerID )
			{
				players[id].screenPos.x = players[playerID].screenPos.x - players[playerID].pos.x + players[id].pos.x;// + (humans[i].pos.w - humans[i].w)/2;
				players[id].screenPos.y = players[playerID].screenPos.y - players[playerID].pos.y + players[id].pos.y;// + (humans[i].pos.h - humans[i].h)/2;
			}
			SDL_RenderCopy( renderer, textures[players[id].body->textureID].texture, &players[id].frame, &players[id].screenPos );
		}
		*/

		if( ShouldIDisplayFPS() )
		{
			for( int i=0;i<sframes.size();i++ )
			{
				SDL_Rect nextNumber = {sframes[i]*10, 0, 10, 18};
				SDL_Rect nextNumberPos = {i*13 + 2, 5, 12, 20};
				SDL_RenderCopy( renderer, numbers.texture, &nextNumber, &nextNumberPos );
			}
			frames++;
		}

        SDL_RenderPresent( renderer );
		SDL_RenderClear( renderer );
    }
	return;
}

void GetMessage()
{
	Uint8 meta[2];
	SDLNet_TCP_Recv( serverSocket, meta, 2 );
	//It will recieve either chars or ints
//	bool intMessage = true;
//	if( meta[0] == 1 or meta[0] == 10 )// texture hash or a texture
//		intMessage = false;

	int message[ meta[1] ];
	int size = meta[1]*4;

	int recieved = 0;
	do
	{
//		std::cout<<recieved<<"/"<<size<<std::endl;
		recieved = SDLNet_TCP_Recv( serverSocket, message+recieved, size-recieved );
	}
	while( recieved < size and recieved > 0 );
	
	if( meta[0] == MSG_META_ID )
		playerID = message[0];
	if( meta[0] == MSG_META_LIST_TEXTURES )
	{
		//texture_t newTexture;
	}
	if( meta[0] == MSG_META_LIST_HUMAN_TEMPLATES )
	{

	}
	/*
	// Recived rest of human templates
	if( meta[0] == 3 )
	{

	}
	*/
	if( meta[0] == MSG_META_LIST_PLAYERS )
	{
		for( int i=0;i<meta[1];i+=4 )
		{
			player_t newPlayer( message[i+1], message[i+2], message[i+3] );
			if( message[i] == playerID )
			{
				//CHANGE
				newPlayer.screenPos.x = 100;
				newPlayer.screenPos.y = 100;
			}
			players.push_back( newPlayer );
			humanIDTable[message[i]] = players.size()-1;
		}
	}
	if( meta[0] == MSG_META_LIST_ROADBLOCK_TEMPLATES )
	{

	}
	if( meta[0] == MSG_META_LIST_ROADBLOCKS )
	{

	}
	if( meta[0] == MSG_META_END_ASSET_DATA )
	{
		recievedInitalInfo = message[0];
	}
	if( meta[0] == MSG_META_TEXTURE )
	{

	}
	if( meta[0] == MSG_META_POSITION_HUMANS )
	{
		for( int i=0;i<meta[1];i+=4 )
		{
			players[ humanIDTable[ message[i] ] ].pos.Set( message[i+1], message[i+2] );
		}
	}
	if( meta[0] == MSG_META_REMOVE_HUMAN )
	{

	}
	if( meta[0] == MSG_META_NEW_HUMAN )
	{
		humanIDTable[ players.size() ] = message[0];
		player_t newPlayer( message[1], message[2], message[3] );
		players.push_back( newPlayer );
	}
	if( meta[0] == MSG_META_REMOVE_ROADBLOCK )
	{

	}

}
