(
	echo '#ifndef __MILO_KEY_H'
	echo '#define __MILO_KEY_H'
	echo 
	echo '/*******************************************************************************'
	echo ' *'
	echo ' * Constant for keys from ncurses to be used globally by milo progream'
	echo ' * for reference to have a standard set of values'
	echo ' */'
	echo 
	echo 'namespace Key {'
	grep '^#define KEY_[A-Z][A-Z]*\s\s*0' /usr/include/ncurses.h | sed 's/\s\s*/ /g' | sed 's/^#define KEY_/    constexpr int /' | sed 's/ 0/=0/' | sed 's/\([0-9]\) /\1; /'
	echo '}'
	echo
	echo '#endif // __MILO_KEY_H'
) > milo_key.h
