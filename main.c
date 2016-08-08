#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

uint8_t *source;
uint64_t source_size = 0;

uint16_t *atoms[512];
uint16_t atoms_size = 0;

uint8_t *sets[512];
uint16_t sets_size = 0;

struct atom_info
    {
        uint16_t scode;
        uint64_t size;
    } atoms_info[512];
uint16_t atoms_info_size = 0;

#include "main.h"

uint8_t Belong(uint8_t data_c, uint8_t *set_ptr)
{

    uint8_t *intervals = set_ptr + *( ( uint16_t*)set_ptr), // included intervals pointer
            *includes_chars = set_ptr + *( (uint16_t*)(set_ptr + 2)), // included chars pointer
           *excluded_chars = set_ptr + *( (uint16_t*)(set_ptr + 4)); // excluded chars pointer


    uint8_t c_belong = 0;

    /* included in interval or in singular chars*/
    uint32_t i;
    for( i = 1; i <= (uint32_t)(*intervals) * 2; i += 2)
    {
        if( *(intervals+i) <= data_c && data_c <= *(intervals+i+1))
        {
            c_belong = 0xff;
            break;
        }
    }

    if( c_belong == 0)
        for( i = 1; i <= (uint32_t)(*includes_chars); i++)
            if( *(includes_chars+i) == data_c)
            {
                c_belong = 0xff;
                break;
            }

    /* singular char */
    if(c_belong == 0) return 0;

    for( i = 1 ; i <= (uint32_t)(*excluded_chars); i++)
        if( *(excluded_chars+i) == data_c)
            return 0;

    return 0xff; // char c belong set
}


int main()
{

    time_t start,stop;

    start = clock();

    FILE *file_in, *file_out;

    file_in = fopen( "source", "r");
    file_out = fopen( "al.out", "w");


    fseek( file_in, 0, SEEK_END);
    source_size = ftell( file_in);
    fseek( file_in, 0, SEEK_SET);



    source = malloc( source_size + 1);
    source[source_size + 1] = 0;

    fread( source, 1, source_size, file_in);


    Init();


    uint64_t current_char = 0;
    while( current_char < source_size)
    {

		uint16_t current_atom;
		for( current_atom = 0; current_atom < atoms_size; current_atom++)
		{

			uint64_t current_char_sec = current_char;
			uint16_t atom_size = atoms[ current_atom][ 1] + 2;
			uint8_t atom_find = 0xff;

			uint16_t atom_char;
			for( atom_char = 2; atom_char < atom_size; atom_char++)
			{
				uint16_t data = atoms[ current_atom][ atom_char];
				uint16_t info = ( data & 0xF000);

				switch( info)
				{
					case 0x0:
						if( current_char_sec >= source_size) { atom_find = 0; break; }

						if( source[ current_char_sec] == ( data & 0xff))
							current_char_sec++;
						else { atom_find = 0; break;}
						break;

					case 0x4000:
						if( source[ current_char_sec] == ( data & 0xff))
                                current_char_sec++;
						break;

					case 0x5000:
						if( current_char_sec >= source_size) { atom_find = 0; break; }

						if( source[ current_char_sec] == ( data & 0xff))
							current_char_sec++;
						else
							{ atom_find = 0; break;}

						while( current_char_sec < source_size && source[ current_char_sec] ==  ( data & 0xff)) ++current_char_sec;
						break;

					case 0x6000:
						while( current_char_sec < source_size && source[ current_char_sec] ==  ( data & 0xff)) ++current_char_sec;
						break;

					case 0x8000:
						if( current_char_sec >= source_size) { atom_find = 0; break; }

						if( Belong( source[ current_char_sec], sets[( data & 0xFFF)]) == 0xff)
							current_char_sec++;
						else
							{ atom_find = 0; break;}
						break;

					case 0xC000:
						if( Belong( source[ current_char_sec], sets[( data & 0xFFF)]) == 0xff)
								current_char_sec++;
						break;

					case 0xD000:
						if( current_char_sec >= source_size) { atom_find = 0; break; }

						if( Belong( source[ current_char_sec], sets[( data & 0xFFF)]) == 0xff)
							current_char_sec++;
						else { atom_find = 0; break; }

						while( current_char_sec < source_size && Belong( source[ current_char_sec], sets[( data & 0xFFF)]) == 0xff)
                                ++current_char_sec;
                        break;
                    case 0xE000:
						while( current_char_sec < source_size && Belong( source[ current_char_sec], sets[( data & 0xFFF)]) == 0xff)
                                ++current_char_sec;
                        break;

					default:
						break;
				}

				if( atom_find == 0) break;

			}


			if( atom_find == 0xff)
			{
				atoms_info[ atoms_info_size].scode = *(atoms[current_atom]);
				atoms_info[ atoms_info_size].size = current_char_sec - current_char;
				atoms_info_size++;
			}
		}


		if( atoms_info_size == 0)
		{
			printf("Error %s\n", source + current_char);
			return 0;
		}

		uint64_t max_atom_size=0;
		uint16_t atom_scode;
		uint16_t atom_info_i;

		for( atom_info_i = 0; atom_info_i < atoms_info_size; ++atom_info_i)
		{
			if( atoms_info[ atom_info_i].size > max_atom_size)
			{
				max_atom_size = atoms_info[ atom_info_i].size;
				atom_scode = atoms_info[ atom_info_i].scode;
			}
		}

		atoms_info_size = 0;

		uint8_t atom[1024];
		atom[ max_atom_size] = 0;
		memcpy( atom, source + current_char, max_atom_size);


		Exec( atom, max_atom_size, atom_scode);
		current_char = current_char + max_atom_size;
	}

	printf("\n\nNanoLex:\tComplete!\n");

    stop = clock();

    printf("%f sec.\n\n",(double)(stop - start)/CLOCKS_PER_SEC);

    return 0;
}
