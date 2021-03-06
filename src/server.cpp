#include <iostream>
#include <SDL2/SDL_net.h>
#include <unordered_set>
#include <thread>

#include "load.hpp"
#include "user.hpp"

#include "class/human.hpp"
#include "class/texture.hpp"
#include "class/message.hpp"

TCPsocket server;
SDLNet_SocketSet allConnectedSockets;

textureBin_t *textures;
hash_t *texturesHash;
humanBlueprint_t *humanBlueprints;
std::vector < user_t > users;
//std::vector < obsticle_t > objects;

void StartServer( Uint16 port, int serverSize )
{
	IPaddress ip;
	SDLNet_Init();
    std::cout<<"Started server on port: "<<port<<" with size: "<<serverSize<<std::endl;
    SDLNet_ResolveHost( &ip, NULL, port );
    allConnectedSockets = SDLNet_AllocSocketSet( serverSize );
    server = SDLNet_TCP_Open( &ip );
	SDLNet_TCP_AddSocket( allConnectedSockets, server );
}

//#include "move.cpp"
//#incluide "ai.cpp"

//void NetRecieve( client_t &sender, int id );
//void NetSendPos();
void NewClient( TCPsocket socket );
//void NetSendNewPlayer( client_t *clientToSend, int id );
//void CheckForReady( client_t *sender, int id );
//void NetSendPlayerList( TCPsocket socket, int id );
const int DEFAULT_PORT = 1234;
const int DEFAULT_SERVER_SIZE = 32;
void ServerMain( Uint16 port = DEFAULT_PORT, int serverSize = DEFAULT_SERVER_SIZE )
{
	std::string level = "Levels/1.lvl";
//	bool succesfulLoad = LoadLevel( level.data() );
//	if( !succesfulLoad )
//		return;
	// Loading
	bool succesfulLoad = true;
	load_t load( level.data() );
	// Dismantle load
	// human blueprints
	humanBlueprints = new humanBlueprint_t [ load.humanBlueprints.size() ];
	for( int i=0; i < load.humanBlueprints.size();i++ )
		humanBlueprints[i].CreateFromArray( load.humanBlueprints[i] );
	
	{
		textures = new textureBin_t [ load.textureFilenames.size() ];
		texturesHash = new hash_t [ load.textureFilenames.size() ];
		int i = 0;
		for( auto it = load.textureFilenames.begin();it != load.textureFilenames.end(); it++ )
		{
			textures[i].LoadBin( it->data() );
			textures[i].id = i;
			texturesHash[i].Hash( textures[i].bin, textures[i].size );
			texturesHash[i].id = i;
			i++;
		}
	}
	
	std::cout<<"Server: Done with loading"<<std::endl;
	StartServer( port, serverSize );

	long long movT = SDL_GetTicks();
	long long chkNetT = SDL_GetTicks();
	long long sendNetPosT = SDL_GetTicks();
	long long infoT = SDL_GetTicks();
	int cycles = 0;
	std::cout<<"Server: Entering main loop"<<std::endl;
	serverIsOn = true;
	while( true )
	{
		// Check for messeges send to server
		if( SDL_GetTicks() - chkNetT >= 0 )
		{
			int numActive = SDLNet_CheckSockets( allConnectedSockets, 0 );
			if( numActive > 0 )
			{
				if( SDLNet_SocketReady( server ) )
				{
					numActive--;
					std::cout<<"server: New client"<<std::endl;
					NewClient( SDLNet_TCP_Accept( server ) );
				}
			}
			/*
			for( int i=0;i<clients.size() and numActive > 0;i++ )
			{
				if( SDLNet_SocketReady( clients[i].socket ) )
				{
					numActive--;
					NetRecieve( clients[i], i );
				}
			}
			*/
			chkNetT = SDL_GetTicks();
		}
		// Send position messeges from server
		if( SDL_GetTicks() - sendNetPosT >= 10 )
		{
			//NetSendPos();
			sendNetPosT = SDL_GetTicks();
		}

		// Update movement
		if( SDL_GetTicks() - movT >= 1000/60 )
		{
			movT = SDL_GetTicks();
		}

		cycles++;
		// Debug or general info goes here
		if( SDL_GetTicks() - infoT >= 1000 )
		{
			//std::cout<<"Cycles per second: "<<cycles<<std::endl;
			cycles = 0;
			infoT = SDL_GetTicks();
		}
	}
}

void NewClient( TCPsocket socket )
{
	SDLNet_TCP_AddSocket( allConnectedSockets, socket );
	user_t newUser( users.size(), socket );
	users.push_back( newUser );
}
void RemoveClient( client_t &someone )
{
	//TODO
	SDLNet_TCP_DelSocket( allConnectedSockets, someone.socket );
}
void NetRecieve( user_t &sender )
{
	msgMeta_t meta;
	int recv = SDLNet_TCP_Recv( sender.socket, &meta, 5 );
	if( recv <= 0 )
	{
		std::cout<<"User has disconnected"<<std::endl;
		RemoveClient( sender );
		return;
	}
	/*

	int msg[meta[1]];
	int size = meta[1]*4;
	int fetched = 0;
	while( fetched < size )
		fetched = SDLNet_TCP_Recv( sender.socket, msg+fetched, size-fetched );

	if( meta[0] == MSG_META_NAME )
	{
		// User has send his username
		//for( int i=0;i<meta[1];i++ )
		//	sender.name[i]=msg[i];
	}
	if( meta[0] == MSG_META_REQ_TEXTURE )
		for( int i=0;i<meta[1];i++ )
			sender.wantsTextureID.push( msg[i] );
	if( meta[0] == MSG_META_REQ_HUMAN_TEMPLATES )
	{
		sender.wantsHumanTemplates = msg[0];
		CheckForReady( &sender, id );
	}
	if( meta[0] == MSG_META_REQ_ROADBLOCK_TEMPLATES )
	{
		sender.wantsRoadblockTemplates = msg[0];
		CheckForReady( &sender, id );
	}
	if( meta[0] == MSG_META_END_ASSET_DATA )
		sender.ready = msg[0];
	if( meta[0] == MSG_META_CHOSEN_CHARACKTER )
	{
		sender.templateID = msg[0];
		CheckForReady( &sender, id );
	}
	if( meta[0] == MSG_META_INPUT )
		sender.Update( msg );
}
void CheckForReady( client_t *sender, int id )
{
	//TODO
	Uint8 meta[2] = {MSG_META_END_ASSET_DATA, 1};
	int message[] = {true};
	SDLNet_TCP_Send( sender->socket, meta, 2 );
	SDLNet_TCP_Send( sender->socket, message, meta[1]*4 );
	NetSendNewPlayer( sender, id );
	NetSendPlayerList( sender->socket, id );
}
void NetSendPos()
{
	int humanPosMsg[ 800 ];
	Uint8 humanPosMsgLen = 0;
	for( int i=0;i<clients.size();i++ )
	{
		if( clients[i].active )
		{
			humanPosMsg[ humanPosMsgLen ] = i;
			humanPosMsg[ humanPosMsgLen+1 ] = clients[i].human.pos.x;
			humanPosMsg[ humanPosMsgLen+2 ] = clients[i].human.pos.y;
			humanPosMsg[ humanPosMsgLen+3 ] = (int)(clients[i].human.drawDirection);
			clients[i].active = false;
			humanPosMsgLen += 4;
		}
	}
	if( humanPosMsgLen > 0 )
	{
		Uint8 posMeta[2] = {MSG_META_POSITION_HUMANS, humanPosMsgLen};
		for( int i=0;i<clients.size();i++ )
		{
			SDLNet_TCP_Send( clients[i].socket, posMeta, 2 );
			SDLNet_TCP_Send( clients[i].socket, humanPosMsg, humanPosMsgLen*4 );
		}
	}
}
void NetSendTexture()
{

}
void NetSendPlayerTemplates()
{

}
void NetSendPlayerList( TCPsocket socket, int idToNotSend )
{
	int message[ clients.size()*4 ];
	int msgSize=0;
	for( int id=0;id<clients.size();id++ )
	{
		if( id != idToNotSend )
		{
			message[msgSize] = id;
			message[msgSize+1] = clients[id].templateID;
			message[msgSize+2] = clients[id].human.pos.x;
			message[msgSize+3] = clients[id].human.pos.y;
			msgSize+=4;
		}
	}
	if( msgSize > 0 )
	{
		Uint8 meta[2] = {MSG_META_LIST_PLAYERS, (Uint8)msgSize};
		SDLNet_TCP_Send( socket, meta, 2 );
		SDLNet_TCP_Send( socket, message, meta[1]*4 );
	}
}
void NetSendNewPlayer( client_t *clientToSend, int id )
{
	Uint8 meta[2] = {MSG_META_NEW_HUMAN, 4};
	int message[] = {id, clientToSend->templateID, (int)clientToSend->human.pos.x, (int)clientToSend->human.pos.y};
	
	for( int i=0;i<clients.size();i++ )
	{
		SDLNet_TCP_Send( clients[i].socket, meta, 2 );
		SDLNet_TCP_Send( clients[i].socket, message, meta[1]*4 );
	}
}
*/
