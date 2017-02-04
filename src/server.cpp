#include<iostream>
#include<SDL2/SDL.h>
#include<SDL2/SDL_net.h>
#include<queue>
#include<vector>
#include<thread>
#include<cmath>
#include<algorithm>
#include<fstream>
#include<time.h>

#include"simpleGeometry.h"
#include"struct.cpp"
#include"init.h"
#include"input.h"
#include"ai.cpp"

SDL_Renderer* renderer;
std::vector<texture_t> textures;

const int MAX_THREAD = 200;
std::vector<flag_t> tempMesh[MAX_THREAD];

std::vector<human_t> humans;
std::vector <obsticle_t> roadblock;

SDL_Rect backgroundPos;
int backgroundTextureID;

std::vector<aoe_t> activeSpells;
std::vector<aoe_t> avalSpells;

int nextAvalHumanID = -1;
int nextAvalTextureID = -1;
int nextAvalThreadID = -1;
int playerID = 0;

int netIDTable[2000];

#include"move.cpp"
#include"load.cpp"

int main()
{
	worker_t threads[MAX_THREAD];

	SDLNet_Init();
	TCPsocket server;
	SDLNet_SocketSet allSockets = SDLNet_AllocSocketSet( 1000 );
	IPaddress ip;
	char address[40];
	Uint16 port = 1234;
	SDLNet_ResolveHost( &ip, NULL, port );
	server = SDLNet_TCP_Open( &ip );
	SDLNet_TCP_AddSocket( allSockets, server );
	
	char level[] = "Levels/1.lvl";
	bool levelLoaded = false;

	long long movT = SDL_GetTicks();
	long long infoT = SDL_GetTicks();
	long long attT = SDL_GetTicks();
	long long spellT = SDL_GetTicks();
	long long checkFlagsT = SDL_GetTicks();
	long long BOTpathFindT = SDL_GetTicks();
	long long BOTattackT = SDL_GetTicks();
	long long BOTvisionT = SDL_GetTicks();
	long long checkNetT = SDL_GetTicks();
	long long sendNetT = SDL_GetTicks();

	long long send=0, recievd = 0;

	human_t humanTemplate;

	int frames = 0;
	std::cout<<"Start"<<std::endl;
	while( true )
	{
		if( !levelLoaded )
		{
			LoadLevel( level );
			humanTemplate = humans[0];
			humanTemplate.eqpSpell = 1;
			humans.clear();
			for( int i=0;i<textures.size();i++ )
				std::cout<<i<<" "<<textures[i].id<<" "<<textures[i].name<<std::endl;
			levelLoaded = true;
		}
		if( SDL_GetTicks() - checkNetT >= 0 )
		{
			int active = SDLNet_CheckSockets( allSockets, 0 );
			while( active > 0 )
			{
				if( SDLNet_SocketReady( server ) )
				{
					TCPsocket newSocket = SDLNet_TCP_Accept( server );
					if( newSocket )
					{
						std::cout<<"New player"<<std::endl;
						active--;
						human_t newGuy = humanTemplate;
						newGuy.socket = newSocket;
						SDLNet_TCP_AddSocket( allSockets, newGuy.socket );
						newGuy.id = ++ nextAvalHumanID;
						newGuy.state = -9;
						newGuy.active = false;

						char msg0[300];
						int msg0Len = 0;
						for( int i=0;i<textures.size();i++ )
						{
							for( int j=0;j<=textures[i].nameLen;j++ )
							{
								msg0[msg0Len] = textures[i].name[j];
								msg0Len++;
							}
							if( msg0Len + textures[i+1].nameLen > 255 )
							{
								Uint8 meta[2] = {0, (Uint8)msg0Len};
								SDLNet_TCP_Send( newGuy.socket, meta, 2 );
								SDLNet_TCP_Send( newGuy.socket, msg0, msg0Len );
								msg0Len = 0;
							}
						}
						if( msg0Len > 0 )
						{
							Uint8 meta[2] = {0, (Uint8)msg0Len};
							SDLNet_TCP_Send( newGuy.socket, meta, 2 );
							SDLNet_TCP_Send( newGuy.socket, msg0, msg0Len );
						}
						
						int msg1[humans.size()*3+20] = { newGuy.id, newGuy.textureID, newGuy.x, newGuy.y, newGuy.w, newGuy.h, newGuy.pos.x, newGuy.pos.y, newGuy.pos.w, newGuy.pos.h, newGuy.frame.x, newGuy.frame.y, newGuy.frame.w, newGuy.frame.h};
						int msg1Len = 14;
						for( int i=0;i<humans.size();i++ )
						{
							msg1[msg1Len] = humans[i].id;
							msg1[msg1Len+1] = humans[i].x;
							msg1[msg1Len+2] = humans[i].y;
							msg1Len+=3;
						}
						Uint8 meta1[2] = {1, (Uint8)msg1Len};
						SDLNet_TCP_Send( newGuy.socket, meta1, 2 );
						SDLNet_TCP_Send( newGuy.socket, msg1, msg1Len*4 );

						int msg2[ roadblock.size()*15 ];
						int msg2Len = 0;
						for( int i=0;i<roadblock.size();i++ )
						{
							msg2[msg2Len] = roadblock[i].id;
							msg2[msg2Len+1] = roadblock[i].textureID;
							msg2[msg2Len+2] = roadblock[i].x;
							msg2[msg2Len+3] = roadblock[i].y;
							msg2[msg2Len+4] = roadblock[i].w;
							msg2[msg2Len+5] = roadblock[i].h;
							msg2[msg2Len+6] = roadblock[i].pos.w;
							msg2[msg2Len+7] = roadblock[i].pos.h;
							msg2[msg2Len+8] = roadblock[i].frame.x;
							msg2[msg2Len+9] = roadblock[i].frame.y;
							msg2[msg2Len+10] = roadblock[i].frame.w;
							msg2[msg2Len+11] = roadblock[i].frame.h;
							msg2Len+=12;
							if( msg2Len > 255 - 12 )
							{
								Uint8 meta2[2] = {2, (Uint8)msg2Len};
								SDLNet_TCP_Send( newSocket, meta2, 2 );
								SDLNet_TCP_Send( newSocket, msg2, msg2Len*4 );
								msg2Len = 0;
							}
						}
						if( msg2Len > 0 )
						{
							Uint8 meta2[2] = {2, (Uint8)msg2Len};
							SDLNet_TCP_Send( newSocket, meta2, 2 );
							SDLNet_TCP_Send( newSocket, msg2, msg2Len*4 );
						}

						Uint8 meta4[2] = {4, 5};
						int msg4[5] = { textures[backgroundTextureID].id, backgroundPos.x, backgroundPos.y, backgroundPos.w, backgroundPos.h };
						SDLNet_TCP_Send( newSocket, meta4, 2 );
						SDLNet_TCP_Send( newSocket, msg4, 20 );

						int msg5[ avalSpells.size()*9 + 1];
						int msg5Len = 0;
						for( int i=0;i<avalSpells.size();i++ )
						{
							msg5[msg5Len] = avalSpells[i].textureID;
							msg5[msg5Len+1] = avalSpells[i].w;
							msg5[msg5Len+2] = avalSpells[i].h;
							msg5[msg5Len+3] = avalSpells[i].pos.w;
							msg5[msg5Len+4] = avalSpells[i].pos.h;
							msg5[msg5Len+5] = avalSpells[i].frame.x;
							msg5[msg5Len+6] = avalSpells[i].frame.y;
							msg5[msg5Len+7] = avalSpells[i].frame.w;
							msg5[msg5Len+8] = avalSpells[i].frame.h;
							msg5Len+=9;
							if( msg5Len > 255 - 8 )
							{
								Uint8 meta5[2] = {5, (Uint8)msg5Len};
								SDLNet_TCP_Send( newSocket, meta5, 2 );
								SDLNet_TCP_Send( newSocket, msg5, msg5Len*4 );
								msg5Len = 0;
							}
						}
						if( msg5Len > 0 )
						{
							Uint8 meta5[2] = {5, (Uint8)msg5Len};
							SDLNet_TCP_Send( newSocket, meta5, 2 );
							SDLNet_TCP_Send( newSocket, msg5, msg5Len*4 );
						}

						Uint8 meta9[2] = {9, 3};
						int msg9[3] = {newGuy.id, newGuy.x, newGuy.y};
						for( int i=0;i<humans.size();i++ )
						{
							SDLNet_TCP_Send( humans[i].socket, meta9, 2 );
							SDLNet_TCP_Send( humans[i].socket, msg9, 12 );
						}

						humans.push_back( newGuy );
					}
				}
				
				for( int i=0;i<humans.size() and active > 0;i++ )
				{
					if( SDLNet_SocketReady( humans[i].socket ) )
					{
						active--;
						Uint8 meta[2];
						int recv = SDLNet_TCP_Recv( humans[i].socket, meta, 2 );
						if( recv > 0 )
						{		
							char msg[ meta[1] ];
							int fetched = 0;
							while( fetched < meta[1] )
								fetched = SDLNet_TCP_Recv( humans[i].socket, msg+fetched, meta[1]-fetched );
							if( meta[0] == 1 )
							{
								humans[i].movDirection[0] = msg[0];
								humans[i].movDirection[1] = msg[1];
								humans[i].attDirection = msg[2];
								humans[i].eqpSpell = msg[3];
								humans[i].active = true;
							}
							if( meta[0] == 2 )
							{
								for( int j=0;j<meta[1];j++ )
								{
									Uint8 myMeta[2] = {3, 1};
									std::cout<<(int)msg[j]<<" "<<textures[ msg[j] ].name<<" "<<textures[ msg[j] ].fileSize<<std::endl;
									SDLNet_TCP_Send( humans[i].socket, myMeta, 2 );
									int size[1] = {textures[ msg[j] ].fileSize};
									SDLNet_TCP_Send( humans[i].socket, size, 4 );
									SDLNet_TCP_Send( humans[i].socket, textures[ msg[j] ].binaryTexture, textures[ msg[j] ].fileSize );
								}
							}
						}else
						{
							std::cout<<"User has disconnected"<<std::endl;
							Uint8 meta11[2] = {11, 1};
							int msg11[1] = {humans[i].id};
							std::swap( humans[i], humans[humans.size()-1] );
							humans.pop_back();
							for( int j=0;j<humans.size();j++ )
							{
								SDLNet_TCP_Send( humans[j].socket, meta11, 2 );
								SDLNet_TCP_Send( humans[j].socket, msg11, 4 );
							}
						}
					}
				}
				active = SDLNet_CheckSockets( allSockets, 0 );
			}

			checkNetT = SDL_GetTicks();
		}
		
		if( SDL_GetTicks() - sendNetT >= 10 ) 
		{
			int msgH[300];
			int msgHLen = 0;
			for( int i=0;i<humans.size();i++ )
			{
				if( humans[i].active )
				{
					msgH[msgHLen] = humans[i].id;
					msgH[msgHLen+1] = humans[i].x;
					msgH[msgHLen+2] = humans[i].y;
					msgH[msgHLen+3] = humans[i].drawDirection;
					humans[i].drawDirection = 0;
					humans[i].active = false;
					msgHLen+=4;
				}
			}
			if( msgHLen > 0 )
			{
				Uint8 meta[2] = {10, (Uint8)msgHLen};
				for( int i=0;i<humans.size();i++ )
				{
					SDLNet_TCP_Send( humans[i].socket, meta, 2 );
					SDLNet_TCP_Send( humans[i].socket, msgH, msgHLen*4 );
				}
			}
			int msgA[300];
			int msgALen = 0;
			for( int i=0;i<activeSpells.size();i++ )
			{
				msgA[msgALen] = activeSpells[i].id;
				msgA[msgALen+1] = activeSpells[i].x;
				msgA[msgALen+2] = activeSpells[i].y;
				msgA[msgALen+3] = (int)activeSpells[i].angle;
				msgALen+=4;
			}
			if( msgALen > 0 )
			{
				Uint8 meta[2] = {12, (Uint8)msgALen};
				for( int i=0;i<humans.size();i++ )
				{
					SDLNet_TCP_Send( humans[i].socket, meta, 2 );
					SDLNet_TCP_Send( humans[i].socket, msgA, msgALen*4 );
				}
			}

			sendNetT = SDL_GetTicks();
		}
		
		if( SDL_GetTicks() - attT >= 1000/60 )
		{
			for( int i=0;i<humans.size();i++ )
				Attack( humans[i] );
			attT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - movT >= 1000/60 )
		{
			for( int i=0;i<humans.size();i++ )
			{
				humans[i].DrawDir();
				Move( humans[i], humans, roadblock );
			}
			movT = SDL_GetTicks();
		}
		
		if( SDL_GetTicks() - spellT >= 10 )
		{
			for( int i = 0;i<activeSpells.size();i++ )
				if( activeSpells[i].duration > 0 )
				{
					std::vector<int> destroyedHumans, destroyedRoadblocks;
					Spell( activeSpells[i], humans, roadblock, destroyedHumans, destroyedRoadblocks );
					int msgR[300];
					int msgRLen = 0;
					for( int j=0;j<destroyedRoadblocks.size();j++ )
					{
						msgR[msgRLen] = roadblock[ destroyedRoadblocks[j] ].id;
						msgRLen++;
						std::swap( roadblock[ roadblock.size()-1 ], roadblock[ destroyedRoadblocks[j] ] );
						roadblock.pop_back();
					}
					if( msgRLen > 0 )
					{
						Uint8 meta[2] = {13, (Uint8)msgRLen};
						for( int j=0;j<humans.size();j++ )
						{
							SDLNet_TCP_Send( humans[j].socket, meta, 2 );
							SDLNet_TCP_Send( humans[j].socket, msgR, msgRLen*4 );
						}
					}

					int msgH[300];
					int msgHLen = 0;
					for( int j=0;j<destroyedHumans.size();j++ )
					{
						msgH[msgHLen] = humans[ destroyedHumans[j] ].id;
						msgHLen++;
						std::swap( humans[j], humans[ destroyedHumans[j] ] );
						humans.pop_back();
					}
					if( msgHLen > 0 )
					{
						Uint8 meta[2] = {7, (Uint8)msgHLen};
						for( int j=0;j<humans.size();j++ )
						{
							SDLNet_TCP_Send( humans[j].socket, meta, 2 );
							SDLNet_TCP_Send( humans[j].socket, msgH, msgHLen*4 );
						}
					}
				}
			for( int j = 0;j < humans.size(); j++ )
				for( int i = 0;i<avalSpells.size();i++ )
					if( humans[j].spellWaitTime[i] > 0 )
						humans[j].spellWaitTime[i]--;
			spellT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - infoT >= 800 )
		{
			//std::cout<<SDL_GetError()<<std::endl;
			//std::cout<<SDLNet_GetError()<<std::endl;
			infoT = SDL_GetTicks();
		}
		for( int i=0;i<activeSpells.size();i++ )
		{
			if( activeSpells[i].duration<=0 )
			{
				std::swap( activeSpells[i], activeSpells[ activeSpells.size()-1 ] );
				activeSpells.pop_back();
			}
		}
	}
}

/*
		if( SDL_GetTicks() - checkFlagsT >= 10 )
		{
			for( int i=0;i<humans.size();i++ )
			{
				if( humans[i].id != humans[playerID].id and threads[humans[i].threadID].done )
				{
					for( int j=0;j<tempMesh[humans[i].threadID].size();j++ )
						humans[i].navMesh.push_back( tempMesh[humans[i].threadID][j] );

					for( int j=0;j<humans[i].navMesh.size();j++ )
					{
						if( humans[i].x == humans[i].navMesh[j].x and humans[i].y == humans[i].navMesh[j].y )
						{
							humans[i].movDirection[0] = humans[i].navMesh[j].goTo[0];
							humans[i].movDirection[1] = humans[i].navMesh[j].goTo[1];
							break;
						}
					}
					if( Distance( humans[i].x, humans[i].y, humans[i].patrolPoint[humans[i].patrolCycle].x, humans[i].patrolPoint[humans[i].patrolCycle].y ) <= 40 )
						humans[i].patrolCycle+=humans[i].switchToNextPoint;
					if( humans[i].cycle )
					{
						if( humans[i].patrolCycle >= humans[i].patrolPoint.size() )
							humans[i].patrolCycle = 0;
					}else
					{
						if( humans[i].patrolCycle >= humans[i].patrolPoint.size()-1 )
							humans[i].switchToNextPoint = -1;
						if( humans[i].patrolCycle <= 0 )
							humans[i].switchToNextPoint = 1;
					}
				
				}
			}
			checkFlagsT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - BOTpathFindT >= 400 )
		{	
			for( int i=0;i<humans.size();i++ )
			{
				if( humans[i].state >= 1 and humans[i].id != humans[playerID].id )
				{
					if( threads[humans[i].threadID].done )
					{
						if( humans[i].state == 1 )
						{
							humans[i].targetX = humans[i].patrolPoint[ humans[i].patrolCycle ].x;
							humans[i].targetY = humans[i].patrolPoint[ humans[i].patrolCycle ].y;
						}
						if( humans[i].state == 2 )
						{
							for( int j=0;j<humans.size();j++ )
							{
								if( humans[i].targetID == humans[j].id )
								{
									humans[i].targetX = humans[j].x;
									humans[i].targetY = humans[j].y;
									break;
								}
							}
						}
						humans[i].navMesh.clear();
						tempMesh[humans[i].threadID].clear();
						threads[humans[i].threadID].done = false;
						if( threads[humans[i].threadID].trd.joinable() )
							threads[humans[i].threadID].trd.join();
						threads[humans[i].threadID].trd = std::thread( PathBuilder, &humans[i], &tempMesh[humans[i].threadID], humans, roadblock, &threads[humans[i].threadID].done, &threads[humans[i].threadID].quit );
					}
				}
				
			}
			BOTpathFindT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - BOTvisionT >= 10 )
		{
			for( int i=0;i<humans.size();i++ )
				if( i != playerID )
					CheckVision( humans[i], humans );
			BOTvisionT = SDL_GetTicks();
		}
		if( SDL_GetTicks() - BOTattackT >= 10 )
		{
			for( int i=0;i<humans.size();i++ )
			{
				if( i!=playerID and humans[i].state == 2 )
				{
						if( (avalSpells[humans[i].eqpSpell].speed == 0 and Distance( humans[i].x, humans[i].y, humans[i].targetX, humans[i].targetY ) < 50 )or avalSpells[humans[i].eqpSpell].speed != 0 )
							humans[i].attDirection = humans[i].prevDrawDirection;
				}
			}
			BOTattackT = SDL_GetTicks();
		}
*/
