/*
*	SmallBASIC - Math RTL
*
*	Nicholas Christopoulos
*/

#include "sys.h"
#include "blib_math.h"
#include "kw.h"
#include "var.h"
#include "device.h"
#include "blib.h"
#include "pproc.h"
#include "smbas.h"

/*
*	length of line
*/
double		line_length(double Ax, double Ay, double Bx, double By)
{
	double	dx, dy;

	dx = Bx-Ax;
	dy = By-Ay;
	return sqrt(dx * dx + dy * dy); 
}

/*
*	Gauss-Jordan method
*/
void	mat_gauss_jordan (double *a, double *b, int n, double toler)
{
	double	big, dum, pivinv, swp;
	int		i, j, k, l, ll;
	int		icol, irow;
	int		*indxc, *indxr, *ipiv;
	
	//
	indxc = (int *) tmp_alloc(sizeof(int) * n);
	indxr = (int *) tmp_alloc(sizeof(int) * n);
	ipiv  = (int *) tmp_alloc(sizeof(int) * n);

	//
	irow = 0;  icol = 0;
	for ( j = 0; j < n; j ++ )
		ipiv[j] = 0;

	for ( i = 0; i < n; i ++ )	{
		big = 0.0;
		for ( j = 0; j < n; j ++ )	{
			if ( ipiv[j] != 1 ) {
				for ( k = 0; k < n; k ++ )	{
					if ( ipiv[k] == 0 ) {
						if ( ABS(a[j*n+k]) >= big )	{
							big = ABS(a[j*n+k]);
							irow = j;
							icol = k;
							}
						}
					else if (ipiv[k] > 1)	{
						err_matsig();
						tmp_free(indxc);
						tmp_free(indxr);
						tmp_free(ipiv);
						return;
						}
					}
				}
			}

		ipiv[icol] = ipiv[icol] + 1;
		if ( irow != icol ) {
	    	for ( l = 0; l < n; l ++ )	
				SWAP(a[irow*n+l], a[icol*n+l], swp);

			SWAP(b[irow], b[icol], swp);
			}

		indxr[i] = irow;
		indxc[i] = icol;

		if (a[icol*n+icol] < toler) {
			err_matsig();
			tmp_free(indxc);
			tmp_free(indxr);
			tmp_free(ipiv);
			return;
			}

		pivinv = 1.0 / a[icol*n+icol];
		a[icol*n+icol] = 1.0;
		for ( l = 0; l < n; l ++ )	
			a[icol*n+l] = a[icol*n+l] * pivinv;

		b[icol] = b[icol] * pivinv;
		for ( ll = 0; ll < n; ll ++ )	{
			if ( ll != icol ) {
				dum = a[ll*n+icol];
				a[ll*n+icol] = 0.0;
				for ( l = 0; l < n; l ++ ) 
				    a[ll*n+l] = a[ll*n+l] - a[icol*n+l] * dum;

				b[ll] = b[ll] - b[icol] * dum;
		     	}
		 	}
		}

	for ( l = n-1; l >= 0; l -- )	{
		if ( indxr[l] != indxc[l]) {
			for ( k = 0; k < n; k ++ )	
				SWAP(a[k*n+indxr[l]], a[k*n+indxc[l]], swp);
			}
		}

	/////////////////
	tmp_free(indxc);
	tmp_free(indxr);
	tmp_free(ipiv);
}

/*
*	Inverse matrix
*/
void	mat_inverse(double *a, int n)
{
	int		i, j, r, t, s, m, l;
	double	*b, *x, p;

	b = tmp_alloc(n * n * sizeof(double));
	x = tmp_alloc(n * n * sizeof(double));
	for ( i = 0; i < n; i ++ )	{
		for ( j = 0; j < n; j ++ )	
			b[i*n+j] = ( i == j );
		}

	for ( r = 0; r < n-1; r ++ )	{
		for ( i = r; i < n-1; i ++ )	{
			p = a[(i+1)*n+r] / a[r*n+r];
			for ( j = r+1; j < n; j ++ )
				a[(i+1)*n+j] = a[(i+1)*n+j] - p * a[r*n+j];
			for ( t = 0; t < n; t ++ )
				b[(i+1)*n+t] = b[(i+1)*n+t] - p * b[r*n+t];
			}
		}

	for ( s = 0; s < n; s ++ )	{
		for ( m = n-1; m >= 0; m -- )	{
			if	( m != n-1 )	{
				for ( l = n-1; l >= m+1; l -- )	
					b[m*n+s] = b[m*n+s] - a[m*n+l] * x[l*n+s];
				}
			x[m*n+s] = b[m*n+s] / a[m*n+m];
			}
		}

	tmp_free(b);
	memcpy(a, x, sizeof(double) * n * n);
	tmp_free(x);
}

/*
*/
void	mat_det2(double t, int m, int k, double *a, int *done, double *v, int n, double toler)
{
	if	( k >= n )  {
		int	s = -1;

		if ((m % 2) == 0)
			s = 1;
		*v += s * t;
		}
	else	{
		if	( ABS(t) > toler ) {
			int n2 = 0, j;

			for ( j = n-1; j >= 0; j -- )	{
				if	( done[j] )
					n2 ++;
				else	{
					done[j] = 1;
					mat_det2( 
						t * a[k*n+j],
						m + n2,
						k + 1,
						a,
						done,
						v,
						n, toler);
					done[j] = 0;
					}
				}
			}
		}
}

/*
*	Determinant of A
*/
double	mat_determ(double *a, int n, double toler)
{
	int		*done;
	int		i;
	double	v;

	done = tmp_alloc(n * sizeof(int));
	for ( i = 0; i < n; i ++ )	
		done[i] = 0;

	v = 0;
	mat_det2(1, 0, 0, a, done, &v, n, toler);

	tmp_free(done);
	return v;
}

/*
*/
double	statmeandev(double *e, int count)
{
	double	sum  = 0.0;
	double	mean;
	int		i;

	if	( count == 0 )
		return 0;

	for ( i = 0; i < count; i ++ )
		sum += e[i];
	mean = sum / count;

	sum = 0.0;
	for ( i = 0; i < count; i ++ )
		sum += fabs(e[i] - mean);

	return sum / count;
}

/*
*/
double	statspreads(double *e, int count)
{
	double	sumsq = 0.0, sum = 0.0;
	int		i;

	if	( count <= 1 )
		return 0;

	for ( i = 0; i < count; i ++ )	{
		sum += e[i];
		sumsq += (e[i] * e[i]);
		}

	return sumsq / (count - 1) - (sum * sum) / (count * (count - 1));
}

/*
*/
double	statspreadp(double *e, int count)
{
	double	sumsq = 0.0, sum = 0.0;
	int		i;

	if	( count <= 0 )
		return 0;

	for ( i = 0; i < count; i ++ )	{
		sum += e[i];
		sumsq += (e[i] * e[i]);
		}

	return sumsq / count - (sum * sum) / (count * count);
}

//
//	INTEGRAL low, high, maxsegs, maxerr, BYREF result, BYREF err USE f(x)
//

/*
double	simpson(double low, double high, int nseg, addr_t use_ip) SEC(BMATH2);
double	simpson(double low, double high, int nseg, addr_t use_ip)
{
	double	width, t, sum, x;
	int		m, j;
	var_t	var;

	v_init(&var);

	width = (high - low) / nseg;

	var.type = V_NUM;	var.n = low;
	exec_usefunc(&var, use_ip);		if	( prog_error )	return 0;
	sum = v_getval(&var);
	v_free(&var);

	var.type = V_NUM;	var.n = high;
	exec_usefunc(&var, use_ip);		if	( prog_error )	return 0;
	sum += v_getval(&var);
	v_free(&var);

	m = nseg >> 1;
	t = 0;
	for ( j = 1; j <= m; j ++ )	{
		x = low + width * (2 * j - 1);

		var.type = V_NUM;	var.n = x;
		exec_usefunc(&var, use_ip);		if	( prog_error )	return 0;
		t += v_getval(&var);
		v_free(&var);
		}
	sum += 4.0 * t;

	m --;
	t = 0;
	for ( j = 1; j <= m; j ++ )	{
		x = low + width * 2.0 * ((double) j);

		var.type = V_NUM;	var.n = x;
		exec_usefunc(&var, use_ip);		if	( prog_error )	return 0;
		t += v_getval(&var);
		v_free(&var);
		}

	sum += 2.0 * t;
	return width * sum / 3.0;
}

void	cmd_integral()
{
	var_t	*res_vp, *err_vp;
	double	low, high, maxerr, errval, oldval;
	int		maxseg, nseg;
	addr_t	use_ip, exit_ip = INVALID_ADDR;

	low = par_getnum();		if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	high = par_getnum();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxseg = par_getint();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxerr = par_getnum();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	res_vp = par_getvar_ptr();	if	( prog_error )	return;
 	par_getcomma();			if	( prog_error )	return;
	err_vp = par_getvar_ptr();	if	( prog_error )	return;

	if	( code_peek() != kwUSE )	{
		rt_raise("INTEGRAL: FUNCTION IS MISSING!");
		return;
		}

	// USE
	code_skipnext();
	use_ip = code_getaddr();
	exit_ip = code_getaddr();

	//
	v_free(res_vp);
	v_free(err_vp);
	res_vp->type = V_NUM;	res_vp->n = 0;
	err_vp->type = V_NUM;	err_vp->n = 0;

	nseg = 10;
	oldval = simpson(low, high, nseg, use_ip);
	if	( !prog_error )	{
		do	{
			nseg = nseg << 1;
			res_vp->n = simpson(low, high, nseg, use_ip);
			if	( prog_error )	break;

			if	( res_vp->n == 0 )
				errval = 0;
			else
				errval = ABS((res_vp->n - oldval) / res_vp->n);
			if ( nseg > maxseg )
				err_vp->n = -errval;
			if ( errval < maxerr )
				err_vp->n = nseg;
			oldval = res_vp->n;
			} while ( err_vp->n == 0.0 );
		}

	code_jump(exit_ip);
}
*/

//
//	ROOT low, high, maxseg, maxerr, BYREF result, BYREF errcode USE ...
//
void	root_iterate(double xl, double xh, double fl, double fh, var_t *res_vp, double maxerr, var_t *err_vp, addr_t use_ip) SEC(BMATH2);
void	root_iterate(double xl, double xh, double fl, double fh, var_t *res_vp, double maxerr, var_t *err_vp, addr_t use_ip)
{
	double	t, x;
	var_t	var;

	v_init(&var);
	do	{
		x = (xl + xh) / 2.0;

		var.type = V_NUM;	var.v.n = x;
		exec_usefunc(&var, use_ip);		if	( prog_error )	return;
		t = v_getval(&var);
		v_free(&var);

		if	( ABS(t) < maxerr )	{
			res_vp->v.n = x;
			err_vp->v.i = 0;
			return;
			}
		else	{
			if ( t * fl > 0.0 )	
				( xl = x, fl = t );
			else
				( xh = x, fh = t );
			}
		} while ( err_vp->v.i );
}

void	cmd_root()
{
	var_t	*res_vp, *err_vp;
	double	low, high, maxerr;
	int		maxseg, nseg;
	addr_t	use_ip, exit_ip = INVALID_ADDR;
	var_t	var;

	int		j;
	double	xl, xh, fl, fh, x, width;

	v_init(&var);
	low = par_getnum();		if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	high = par_getnum();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxseg = par_getint();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxerr = par_getnum();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	res_vp = par_getvar_ptr();	if	( prog_error )	return;
 	par_getcomma();			if	( prog_error )	return;
	err_vp = par_getvar_ptr();	if	( prog_error )	return;

	if	( code_peek() != kwUSE )	{
		rt_raise("INTEGRAL: FUNCTION IS MISSING!");
		return;
		}

	// USE
	code_skipnext();
	use_ip = code_getaddr();
	exit_ip = code_getaddr();

	//
	v_free(res_vp);
	v_free(err_vp);
	res_vp->type = V_NUM;	res_vp->v.n = 0;
	err_vp->type = V_INT;	err_vp->v.i = 1;

	xl = low;
	xh = high;

	var.type = V_NUM;	var.v.n = xl;
	exec_usefunc(&var, use_ip);		if	( prog_error )	return;
	fl = v_getval(&var);
	v_free(&var);

	var.type = V_NUM;	var.v.n = xh;
	exec_usefunc(&var, use_ip);		if	( prog_error )	return;
	fh = v_getval(&var);
	v_free(&var);

	if ( ABS(fl) < maxerr )	{
		err_vp->v.i = 0;
		res_vp->v.n = xl;
		code_jump(exit_ip);
		return;
		}

	if	( ABS(fh) < maxerr ) {
		err_vp->v.i = 0;
		res_vp->v.n = xh;
		code_jump(exit_ip);
		return;
		}

	if ( fl * fh < 0 )	{
		root_iterate(xl, xh, fl, fh, res_vp, maxerr, err_vp, use_ip);
		code_jump(exit_ip);
		return;
		}

	nseg = 2;
	do	{
		width = (xh - xl) / nseg;
		for ( j = 1; j <= (nseg >> 1); j ++ )	{
			x = xl + width * (2 * j - 1);

			var.type = V_NUM;	var.v.n = x;
			exec_usefunc(&var, use_ip);		if	( prog_error )	return;
			fh = v_getval(&var);
			v_free(&var);

			if	( fh * fl < 0 )	{
				xh = x;
				root_iterate(xl, xh, fl, fh, res_vp, maxerr, err_vp, use_ip);
				code_jump(exit_ip);
				return;
				}
			}
		nseg = nseg << 1;
		} while ( nseg <= maxseg );

	code_jump(exit_ip);
}

//
//	DERIV x, maxtries, maxerr, BYREF result, BYREF errcode USE ...
//
void	cmd_deriv()
{
	var_t	*res_vp, *err_vp;
	double	maxerr, x;
	int		maxseg, nseg;
	addr_t	use_ip, exit_ip = INVALID_ADDR;
	var_t	var;

	double	delta = 0.01, f1, f2, f3, fp, op = 0.0, errval;

	v_init(&var);
	x = par_getnum();		if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxseg = par_getint();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxerr = par_getnum();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	res_vp = par_getvar_ptr();	if	( prog_error )	return;
 	par_getcomma();			if	( prog_error )	return;
	err_vp = par_getvar_ptr();	if	( prog_error )	return;

	if	( code_peek() != kwUSE )	{
		rt_raise("INTEGRAL: FUNCTION IS MISSING!");
		return;
		}

	// USE
	code_skipnext();
	use_ip = code_getaddr();
	exit_ip = code_getaddr();

	//
	v_free(res_vp);
	v_free(err_vp);
	res_vp->type = V_NUM;	res_vp->v.n = 0;
	err_vp->type = V_INT;	err_vp->v.i = 1;

	nseg = 0;
	do	{
		var.type = V_NUM;	var.v.n = x;
		exec_usefunc(&var, use_ip);		if	( prog_error )	return;
		f1 = v_getval(&var);
		v_free(&var);

		var.type = V_NUM;	var.v.n = x + delta;
		exec_usefunc(&var, use_ip);		if	( prog_error )	return;
		f2 = v_getval(&var);
		v_free(&var);

		var.type = V_NUM;	var.v.n = x + 2 * delta;
		exec_usefunc(&var, use_ip);		if	( prog_error )	return;
		f3 = v_getval(&var);
		v_free(&var);

		fp = (-3.0 * f1 + 4.0 * f2 - f3)/2.0/delta;
		if	( fp == 0.0 )
			errval = 0;
		else
			errval = ABS((fp-op)/fp);

		nseg ++;
		if	( nseg >= maxseg )	{
			res_vp->v.n = fp;
			break;
			}

		if ( errval < maxerr )	{
			res_vp->v.n = fp;
			err_vp->v.i = 0;
			}

		delta = delta / 2.0;
		op = fp;

		} while ( err_vp->v.i );

	code_jump(exit_ip);
}

//
//	DIFFEQ x0, y0, x, maxseg, maxerr, BYREF y, BYREF errcode USE ...
//
void	cmd_diffeq()
{
	var_t	*res_vp, *err_vp;
	double	maxerr;
	int		maxseg, nseg, j;
	addr_t	use_ip, exit_ip = INVALID_ADDR;
	var_t	var, var2;

	double	x0, x1, y0, width, hw, ka, kb, kc, kd, xi, yi;
	double	errval, yp = 0;

	v_init(&var);
	v_init(&var2);
	x0 = par_getnum();		if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	y0 = par_getnum();		if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	x1 = par_getnum();		if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxseg = par_getint();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	maxerr = par_getnum();	if	( prog_error )	return;
	par_getcomma();			if	( prog_error )	return; 
	res_vp = par_getvar_ptr();	if	( prog_error )	return;
 	par_getcomma();			if	( prog_error )	return;
	err_vp = par_getvar_ptr();	if	( prog_error )	return;

	if	( code_peek() != kwUSE )	{
		rt_raise("INTEGRAL: FUNCTION IS MISSING!");
		return;
		}

	// USE
	code_skipnext();
	use_ip = code_getaddr();
	exit_ip = code_getaddr();

	//
	v_free(res_vp);
	v_free(err_vp);
	res_vp->type = V_NUM;	res_vp->v.n = 0;
	err_vp->type = V_INT;	err_vp->v.i = 1;

	nseg = 1;
	do	{
		width = (x1 - x0) / nseg;
		hw = width / 2.0;

		for ( j = 1; j <= nseg; j ++ )	{
			xi = x0 + (j-1) * width;
//			xf = xi + width;
			if ( j == 1 )
				yi = y0;
			else
				yi = res_vp->v.n;

			var.type  = V_NUM;	var.v.n  = xi;
			var2.type = V_NUM;	var2.v.n = yi;
			exec_usefunc2(&var, &var2, use_ip);		if	( prog_error )	return;
			ka = v_getval(&var);
			v_free(&var);
			
			var.type  = V_NUM;	var.v.n  = xi + hw;
			var2.type = V_NUM;	var2.v.n = yi + ka * hw;
			exec_usefunc2(&var, &var2,  use_ip);		if	( prog_error )	return;
			kb = v_getval(&var);
			v_free(&var);

			var.type  = V_NUM;	var.v.n  = xi + hw;
			var2.type = V_NUM;	var2.v.n = yi + kb * hw;
			exec_usefunc2(&var, &var2,  use_ip);		if	( prog_error )	return;
			kc = v_getval(&var);
			v_free(&var);

			var.type  = V_NUM;	var.v.n  = xi + width;
			var2.type = V_NUM;	var2.v.n = yi + kc * width;
			exec_usefunc2(&var, &var2,  use_ip);		if	( prog_error )	return;
			kd = v_getval(&var);
			v_free(&var);

			res_vp->v.n = yi + width * (ka + 2.0 * kb + 2.0 * kc + kd) / 6.0;
			}

		if	( nseg == 1 )
			errval = maxerr;
		else	{
			if	( res_vp->v.n == 0 )
				errval = 0;
			else
				errval = ABS((res_vp->v.n - yp) / res_vp->v.n);
			}
		
		if ( nseg > maxseg )	
			break;

		if	( errval < maxerr || width == 0 )	{
			err_vp->v.i = 0;
			break;
			}

		yp = res_vp->v.n;
		nseg = nseg << 1;
		} while ( err_vp->v.i );

	code_jump(exit_ip);
}
