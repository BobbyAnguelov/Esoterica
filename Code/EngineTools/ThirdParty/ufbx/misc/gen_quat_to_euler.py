import sympy
from sympy.matrices import Matrix
from sympy.algebras.quaternion import Quaternion

orders = "XYZ XZY YZX YXZ ZXY ZYX".split()

unit_vectors = [
    (1,0,0),
    (0,1,0),
    (0,0,1),
]

def rotate_point(q, v):
    qv = Matrix([q.b, q.c, q.d])
    qw = q.a
    vv = Matrix(v)

    # https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/
    t = (2.0*qv).cross(vv)
    r = vv + qw*t + qv.cross(t)
    return (r[0], r[1], r[2])

# "Quaternion to Euler Angle Conversion for Arbitrary Rotation Sequence Using Geometric Methods"
# http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/nhughes/quat_2_euler_for_MB.pdf
def solve_euler_t01(quat, order):
    i0,i1,i2 = order
    i0n = (i0+1)%3
    i0nn = (i0+2)%3
    v2 = unit_vectors[i2]

    l = quat.a*quat.a + quat.b*quat.b + quat.c*quat.c + quat.d*quat.d

    v2r = tuple(c.subs(l, 1) for c in Quaternion.rotate_point(v2, quat))

    if (i0+1) % 3 == i1:
        t0 = sympy.atan2(-v2r[i0n].factor(), (v2r[i0nn] + l).factor().subs(l, 1) - 1)
        t1 = v2r[i0].factor()
    else:
        t0 = sympy.atan2(v2r[i0nn].factor(), (v2r[i0n] + l).factor().subs(l, 1) - 1)
        t1 = -v2r[i0].factor()

    return (t0, t1)

def solve_euler_t2_fallback(quat, order, t):
    i0,i1,i2 = order
    i0n = (i0+1)%3
    i0nn = (i0+2)%3
    v0 = unit_vectors[i0]

    l = quat.a*quat.a + quat.b*quat.b + quat.c*quat.c + quat.d*quat.d

    v0r = tuple(c.subs(l, 1) for c in Quaternion.rotate_point(v0, quat))

    if (i0+1) % 3 == i1:
        t2 = sympy.atan2(t*v0r[i0n].factor(), -t*((v0r[i0nn] + l).factor().subs(l, 1) - 1))
    else:
        t2 = sympy.atan2(t*v0r[i0nn].factor(), t*((v0r[i0n] + l).factor().subs(l, 1) - 1))

    return t2

qx,qy,qz,qw = sympy.symbols("qx qy qz qw")
quat = Quaternion(qw,qx,qy,qz)

def format_c(expr, prec=0):
    if expr.is_Add:
        terms = sorted((format_c(a, 1) for a in expr.args),
            key=lambda s: s.startswith("-"))
        args = []
        for term in terms:
            if len(args) > 0:
                if term.startswith("-"):
                    args.append("-")
                    term = term[1:]
                else:
                    args.append("+")
            args.append(term)
        args = " ".join(args)
        return f"({args})" if prec > 1 else args
    elif expr.is_Mul:
        if expr.args[0] == -1:
            args = "*".join(format_c(a, 2) for a in expr.args[1:])
            return f"-({args})" if prec > 2 else f"-{args}"
        else:
            args = "*".join(format_c(a, 2) for a in expr.args)
            return f"({args})" if prec > 2 else args
    elif expr.is_Function:
        args = ", ".join(format_c(a, 0) for a in expr.args)
        name = expr.func.__name__
        if name == "asin":
            name = "ufbxi_asin"
        return f"{name}({args})"
    elif expr.is_Pow:
        base, exp = expr.args
        if base.is_Symbol and exp == 2:
            b = format_c(base, 2)
            return f"({b}*{b})" if prec > 2 else f"{b}*{b}"
        else:
            return f"pow({format_c(base, 0)}, {format_c(exp, 0)})"
    elif expr.is_Integer:
        return f"{int(expr.evalf())}.0f"
    elif expr.is_Symbol:
        return expr.name
    else:
        raise TypeError(f"Unhandled type {type(expr)}")

def format_code(expr):
    return format_c(expr)
    # return sympy.ccode(expr).replace("asin", "ufbxi_asin")

print("\tswitch (order) {")
for order_s in orders:
    order = tuple(reversed(tuple("XYZ".index(o) for o in order_s)))
    t0, t1 = solve_euler_t01(quat, order)

    rev_order = tuple(reversed(order))
    rev_quat = Quaternion.conjugate(quat)
    t2n, _ = solve_euler_t01(rev_quat, rev_order)

    t2f = solve_euler_t2_fallback(quat, order, sympy.Symbol("t"))

    ts = (t0, t1, -t2n)

    print("\tcase UFBX_ROTATION_{}:".format(order_s))
    c0 = "xyz"[order[0]]
    c1 = "xyz"[order[1]]
    c2 = "xyz"[order[2]]
    e0 = format_code(t0)
    e1 = format_code(t1)
    e2 = format_code(-t2n)
    f2 = format_code(t2f)
    
    print(f"\t\tt = {e1};")
    print(f"\t\tif (fabs(t) < eps) {{")
    print(f"\t\t\tv.{c1} = (ufbx_real)asin(t);")
    print(f"\t\t\tv.{c0} = (ufbx_real){e0};")
    print(f"\t\t\tv.{c2} = (ufbx_real){e2};")
    print("\t\t} else {")
    print(f"\t\t\tv.{c1} = (ufbx_real)copysign(UFBXI_DPI*0.5, t);")
    print(f"\t\t\tv.{c0} = (ufbx_real)({f2});")
    print(f"\t\t\tv.{c2} = 0.0f;")
    print("\t\t}")

    print("\t\tbreak;")
print("\tdefault:")
print("\t\tv.x = v.y = v.z = 0.0;")
print("\t\tbreak;")
print("\t}")    
