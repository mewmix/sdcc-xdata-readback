/* C-only positive control: an independent volatile destination read. */
unsigned char load_cell(volatile __xdata unsigned char *p)
{
    return *p;
}
