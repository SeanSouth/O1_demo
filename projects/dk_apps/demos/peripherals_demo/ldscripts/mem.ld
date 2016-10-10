MEMORY
{
        ROM (rx) : ORIGIN = 0x7FC0000, LENGTH = ( 84 * 1024)
        RetRAM0 (rwx): ORIGIN = 0x7FC0000 + ( 84 * 1024) + 0, LENGTH = (( 44 * 1024) - 0)
        RetRAM1 (rwx): ORIGIN = (0), LENGTH = 0
        RAM (rw) : ORIGIN = 0x7FC0000 + ( 84 * 1024) + ((( 44 * 1024) - 0) + 0), LENGTH = ( 16 * 1024)
}
