#include <stdio.h>
#include <stdlib.h>

#include "map.h"
#include "point.h"
#include "types.h"
#include "queue.h"
#include "sorted_queue.h"

#define MAX_NCOLS 64           /* Maximum map cols*/
#define MAX_NROWS 64           /* Maximum map rows*/
#define MAX_BUFFER 64          /* Maximum file line size*/
#define MAX_STACK_SIZE 64 * 64 /* Maximum stack size */
#define NUM_NEIGHBORS 4        /* Ignoramos la posición STAY */

struct _Map
{
    unsigned int nrows, ncols;
    Point *array[MAX_NROWS][MAX_NCOLS]; /* array with the Map points*/
    Point *input, *output;              /* points input/output*/
};

struct _Point
{
    int x, y;
    char symbol;
    Bool visited; /* for DFS*/
};

struct _Stack
{
    void *data[MAX_STACK_SIZE];
    int top;
};

Map *map_new(unsigned int nrows, unsigned int ncols)
{
    Map *map;
    int i, j;

    /* Comprobamos que los valores de las filas y las columnas es correcto */
    if (nrows > MAX_NROWS || ncols > MAX_NCOLS)
    {
        return NULL;
    }

    /* Reservamos memoria para el mapa*/
    map = (Map *)calloc(1, sizeof(Map));

    map->nrows = nrows;
    map->ncols = ncols;

    /* Inicializamos el mapa con un símbolo cualquiera */
    for (i = 0; i < nrows; i++)
    {
        for (j = 0; j < ncols; j++)
        {
            map->array[i][j] = point_new(j, i, BARRIER);
        }
    }

    /* Inicializamos los valores del input y del output */
    map->input = point_new(0, 0, INPUT);
    map->output = point_new(0, 1, OUTPUT);

    return map;
}

void map_free(Map *mp)
{
    int i, j;
    unsigned int nrows, ncols;

    nrows = mp->nrows;
    ncols = mp->ncols;

    /* Liberamos cada punto del mapa */
    for (i = 0; i < nrows; i++)
    {
        for (j = 0; j < ncols; j++)
        {
            point_free(mp->array[i][j]);
        }
    }

    point_free(mp->output);
    point_free(mp->input);
    free(mp);
}

Point *map_insertPoint(Map *mp, Point *p)
{
    if (mp == NULL || p == NULL)
    {
        return NULL;
    }

    /* Insertamos el punto en el mapa a partir de sus coordenadas*/
    mp->array[p->y][p->x] = p;

    /* Devolvemos el punto */
    return mp->array[p->y][p->x];
}

int map_getNcols(const Map *mp)
{
    if (mp == NULL)
    {
        return -1;
    }

    return mp->ncols;
}

int map_getNrows(const Map *mp)
{
    if (mp == NULL)
    {
        return -1;
    }

    return mp->nrows;
}

Point *map_getInput(const Map *mp)
{
    if (mp == NULL)
    {
        return NULL;
    }

    return mp->input;
}

Point *map_getOutput(const Map *mp)
{
    if (mp == NULL)
    {
        return NULL;
    }

    return mp->output;
}

Point *map_getPoint(const Map *mp, const Point *p)
{
    if (mp == NULL || p == NULL)
    {
        return NULL;
    }

    /* Return el punto con las mismas coordenadas que p*/
    return mp->array[p->y][p->x];
}

Point *map_getNeighboor(const Map *mp, const Point *p, Position pos)
{
    if (mp == NULL || p == NULL)
    {
        return NULL;
    }

    /* Hacemos return del vecino según la posición*/
    switch (pos)
    {
    case RIGHT:
        return mp->array[p->y][p->x + 1];

    case UP:
        return mp->array[p->y - 1][p->x];

    case LEFT:
        return mp->array[p->y][p->x - 1];

    case DOWN:
        return mp->array[p->y + 1][p->x];

    case STAY:
        return mp->array[p->y][p->x];

    default:
        return NULL;
        break;
    }
}

Status map_setInput(Map *mp, Point *p)
{
    if (mp == NULL || p == NULL)
    {
        return ERROR;
    }

    mp->input = p;

    return OK;
}

Status map_setOutput(Map *mp, Point *p)
{
    if (mp == NULL || p == NULL)
    {
        return ERROR;
    }

    mp->output = p;

    return OK;
}

Map *map_readFromFile(FILE *pf)
{
    /*Declaramos las variables internas de la funcion*/
    Map *mp = NULL;
    char sec;
    int j, i;

    if (!pf)
        return NULL;
    /*Reservamos memoria para un mapa*/

    mp = (Map *)calloc(1, sizeof(Map));

    if (!mp)
        return NULL;

    /*Se comienza a leer el archivo mapa*/

    if (fscanf(pf, "%u %u\n", &mp->nrows, &mp->ncols) != 2)
    {
        free(mp);
        return NULL;
    }

    /*Leemos todos los puntos del mapa*/

    for (i = 0; i < mp->nrows; i++)
    {
        for (j = 0; j < mp->ncols; j++)
        {
            /*Error durante lectura*/
            if (fscanf(pf, "%c", &sec) != 1)
            {
                /*Liberamos elementos en caso de error*/
                for (j--; j >= 0; j--)
                    free(mp->array[i][j]);

                for (i--; i >= 0; i--)
                {
                    for (j = mp->ncols - 1; j >= 0; j--)
                        free(mp->array[i][j]);
                }
                /*Liberamos mapa*/
                free(mp);
                return NULL;
            }
            /*Creamos el nuevo punto*/

            mp->array[i][j] = point_new(j, i, sec);

            /*Si se produce algún error liberamos memoria*/

            if (mp->array[i][j] == NULL)
            {
                for (j--; j >= 0; j--)
                    free(mp->array[i][j]);

                for (i--; i >= 0; i--)
                {
                    for (j = mp->ncols - 1; j >= 0; j--)
                        free(mp->array[i][j]);
                }
                free(mp);
                return NULL;
            }

            /*Asignamos input y output*/

            if (sec == OUTPUT)
                map_setOutput(mp, mp->array[i][j]);
            else if (sec == INPUT)
                map_setInput(mp, mp->array[i][j]);
        }

        fscanf(pf, "\n");
    }

    return mp;
}

Bool map_equal(const void *_mp1, const void *_mp2)
{
    int nrows1, nrows2, ncols1, ncols2, i, j;

    /* Casteamos el tipo void * para transformarlo en un tipo Map * */
    Map *mp1 = (Map *)_mp1;
    Map *mp2 = (Map *)_mp2;

    if (mp1 == NULL || mp2 == NULL)
    {
        return FALSE;
    }

    ncols1 = mp1->ncols;
    nrows1 = mp1->nrows;
    ncols2 = mp2->ncols;
    nrows2 = mp2->nrows;

    /* Si el número de filas y columnas no es idéntico, entonces los mapas no son iguales*/
    if (nrows1 != nrows2 || ncols1 != ncols2)
    {
        return FALSE;
    }

    if (point_equal(mp1->input, mp2->input) == FALSE || point_equal(mp1->output, mp2->output) == FALSE)
    {
        return FALSE;
    }

    /* Si hay un solo punto que sea distinto return FALSE */
    for (i = 0; i < nrows1; i++)
    {
        for (j = 0; j < ncols1; j++)
        {
            if (point_equal(mp1->array[i][j], mp2->array[i][j]) == FALSE)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

int map_print(FILE *pf, Map *mp)
{
    int i, j, c;

    if (pf == NULL || mp == NULL)
    {
        return -1;
    }

    printf("%u, %u\n", mp->nrows, mp->ncols);

    for (i = 0; i < mp->nrows; i++)
    {
        for (j = 0; j < mp->ncols; j++)
        {
            /* Utilizamos += porque el return de la función si todo se ha ejecutado correctamente es el número de caracteres impresos */
            c += point_print(stdout, mp->array[i][j]);
        }
    }
    return c;
}

int point_cmp(const Point *p1, const Point *p2)
{
    int x1, x2, y1, y2;

    if (!p1 || !p2)
        return -1;

    x1 = point_getCoordinateX(p1);
    x2 = point_getCoordinateX(p2);
    y1 = point_getCoordinateY(p1);
    y2 = point_getCoordinateY(p2);

    if (x1 == x2 && y1 == y2)
        return 1;

    return 0;
}

Point *map_bfs(FILE *pf, Map *mp)
{
    Queue *q = NULL;
    Point *input = NULL, *output = NULL, *ele = NULL, *aux = NULL;
    Status st = OK;
    Position pos;
    int f = 0;

    if (!pf || !mp)
        return NULL;

    /*Inicializamos una cola auxiliar */
    q = queue_new();

    if (!q)
        return NULL;

    /* Insertamos input en la cola auxiliar */
    input = map_getInput(mp);

    st = queue_push(q, input);

    if (!st)
    {
        queue_free(q);
        return NULL;
    }

    /* Hallamos el output */
    output = map_getOutput(mp);

    /* Mientras la cola no esté vacía */
    while ((queue_isEmpty(q)) == FALSE && (f == 0))
    {
        /* Extraer el punto de la cola y marcarlo como visitado */
        ele = (Point *)queue_pop(q);

        /* Si el punto extraido no es el punto de llegada y no ha sido
    visitado, explorar sus vecinos */

        if (point_getVisited(ele) == FALSE)
        {

            point_setVisited(ele, TRUE);
            point_print(pf, ele);

            if (point_cmp(ele, output) == 1)
                f = 1;
            else
            {
                for (pos = 0; pos < 4; pos++)
                {

                    aux = map_getNeighboor(mp, ele, pos);

                    if (!aux)
                    {
                        queue_free(q);
                        return NULL;
                    }

                    if ((point_getVisited(aux) != TRUE) && point_getSymbol(aux) != BARRIER && point_getSymbol(aux) != INPUT)
                    {

                        st = queue_push(q, aux);

                        if (!st)
                        {
                            queue_free(q);
                            return NULL;
                        }
                    }
                }
            }
        }
    }
    queue_free(q);
    return output;
}
