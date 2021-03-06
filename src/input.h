#include<fstream>
const Uint8 *keyboardState = SDL_GetKeyboardState( NULL );
struct input_t
{
	bool active = false;
	int scanCode;
	void reset()
	{
		active = false;
	}
	void check()
	{
		active = keyboardState[scanCode];
	}
	input_t( int code )
	{
		scanCode = code;
	}
};
std::vector<input_t> input;
void InitInput()
{
	std::ifstream keyLayout;
	keyLayout.open( "Configs/keyboard.cfg" );
	while( !keyLayout.eof() )
	{
		int temp;
		keyLayout>>temp;
		input_t toPush(temp);
		input.push_back( toPush );
	}
}
void ScanKeyboard()
{
	for( int i=0;i<input.size();i++ )
		input[i].check();
}
int spellchngTimeout = 0;
void AnalyzeInput( int *result, bool &keyPressed )
{
	for( int i=0;i<input.size();i++ )
		if( input[i].active )
			keyPressed = true;
	if( input[0].active )
		result[0] = 'n';
	if( input[1].active )
		result[0] = 's';
	if( input[2].active )
		result[1] = 'w';
	if( input[3].active )
		result[1] = 'e';
	if( input[4].active )
		result[2] = 'n';
	if( input[5].active )
		result[2] = 's';
	if( input[6].active )
		result[2] = 'w';
	if( input[7].active )
		result[2] = 'e';
//	if( input[8].active)
//		someone.speed = someone.normSpeed*1.8;
/*
	if( input[9].active and spellchngTimeout == 0 )
	{
		someone.curSpellNum++;
		spellchngTimeout = 18;
		if( someone.curSpellNum == someone.avalSpells.size() )
			someone.curSpellNum = 0;
	}
	if( input[10].active and spellchngTimeout == 0 )
	{
		someone.curSpellNum--;
		spellchngTimeout = 18;
		if( someone.curSpellNum == -1 )
			someone.curSpellNum = someone.avalSpells.size()-1;
	}
	if( spellchngTimeout > 0 )
		spellchngTimeout--;
	if( !someone.avalSpells.empty() )
		someone.eqpSpell = someone.avalSpells[someone.curSpellNum];
		*/

}
void ResetKeyboard()
{
	for( int i=0;i<input.size();i++ )
		input[i].reset();
}
