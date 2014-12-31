#ifndef KTOOLS_ALGEBRA_HPP
#define KTOOLS_ALGEBRA_HPP

#include "ktools_common.hpp"
#include "metaprogramming.hpp"

namespace KTools {
	template<size_t N, typename T>
	class Vector;

	template<size_t n, typename T>
	class SquareMatrix;

	template<size_t n, typename T>
	class ProjectiveMatrix; 


	template<size_t N, typename T = float_type>
	class Vector {
		template<size_t M, typename U>
		friend class Vector;

		template<size_t M, typename U>
		friend class SquareMatrix;

		template<size_t M, typename U>
		friend class ProjectiveMatrix;

	protected:
		T x[N];

	public:
		typedef T value_type;
		static const size_t dimension = N;

		Vector() {
			for(size_t i = 0; i < N; i++) {
				x[i] = T();
			}
		}
		// no init constructor.
		Vector(Nil) {}
		Vector(T pad) {
			for(size_t i = 0; i < N; i++) {
				x[i] = pad;
			}
		}

		Vector& operator=(const Vector& v) {
			for(size_t i = 0; i < N; i++) {
				x[i] = v.x[i];
			}
			return *this;
		}

		Vector(const Vector& v) {
			*this = v;
		}

		Vector copy() const {
			return Vector(*this);
		}

		T& operator[](size_t i) {

			return x[i];
		}

		const T& operator[](size_t i) const {
			return x[i];
		}

		Vector& operator+=(const Vector& v) {
			for(size_t i = 0; i < N; i++) {
				x[i] += v.x[i];
			}
			return *this;
		}

		Vector operator+(const Vector& v) const {
			return Vector(*this) += v;
		}

		Vector& operator*=(T lambda) {
			for(size_t i = 0; i < N; i++) {
				x[i] *= lambda;
			}
			return *this;
		}

		Vector operator*(T lambda) const {
			return Vector(*this) *= lambda;
		}

		friend Vector operator*(T lambda, const Vector& v) {
			return Vector(v) *= lambda;
		}

		Vector& negate() {
			return (*this) *= -1;
		}

		Vector operator-() const {
			return Vector(*this).negate();
		}

		T inner(const Vector& v) const {
			T ret = T();
			for(size_t i = 0; i < N; i++) {
				ret += x[i]*v.x[i];
			}
			return ret;
		}

		// inner
		T operator*(const Vector& v) const {
			return inner(v);
		}

		template<size_t M>
		Vector<M, T> extend(T pad = T()) const {
			Vector<M, T> v = nil;
			for(size_t i = 0; i < (M < N ? M : N); i++) {
				v.x[i] = x[i];
			}
			for(size_t i = N; i < M; i++) {
				v.x[i] = pad;
			}
			return v;
		}
	};

	template<size_t n, typename T = float_type>
	class SquareMatrix : public Vector<n*n, T> {
		template<size_t M, typename U>
		friend class Vector;

		template<size_t M, typename U>
		friend class SquareMatrix;

		template<size_t M, typename U>
		friend class ProjectiveMatrix;

	protected:
		typedef Vector<n*n, T> super;

	public:
		// For matrix-vector multiplication.
		typedef Vector<n, T> vector_type;

		SquareMatrix& setDiagonal(T diag = T(1)) {
			for(size_t i = 0; i < n; i++) {
				(*this)[i][i] = diag;
			}
			return *this;
		}

		SquareMatrix() : super() {
			setDiagonal(1);
		}
		SquareMatrix(Nil) : super(nil) {}
		explicit SquareMatrix(T diag) : super() {
			setDiagonal(diag);
		}
		SquareMatrix(const SquareMatrix& A) : super(A) {}

		SquareMatrix& operator=(const SquareMatrix& A) {
			super::operator=(A);
			return *this;
		}

		SquareMatrix copy() const {
			return SquareMatrix(*this);
		}

		static size_t rows() {
			return n;
		}

		static size_t columns() {
			return n;
		}

		T* operator[](size_t i) {
			return (this->x + n*i);
		}

		const T* operator[](size_t i) const {
			return (this->x + n*i);
		}

		SquareMatrix& operator+=(const SquareMatrix& v) {
			super::operator+=(v);
			return *this;
		}

		SquareMatrix operator+(const SquareMatrix& v) const {
			return SquareMatrix(*this) += v;
		}

		SquareMatrix& operator*=(T lambda) {
			super::operator*=(lambda);
			return *this;
		}

		SquareMatrix operator*(T lambda) const {
			return SquareMatrix(*this) *= lambda;
		}

		friend SquareMatrix operator*(T lambda, const SquareMatrix& M) {
			return SquareMatrix(M) *= lambda;
		}

	protected:
		static void multiply(const SquareMatrix& A, const SquareMatrix& B, SquareMatrix& result) {
			for(size_t i = 0; i < n; i++) {
				const T* Arow = A[i];
				T* Rrow = result[i];
				for(size_t j = 0; j < n; j++) {
					Rrow[j] = 0;
					for(size_t k = 0; k < n; k++) {
						Rrow[j] += Arow[k]*B[k][j];
					}
				}
			}
		}

	public:
		SquareMatrix& operator*=(const SquareMatrix& M) {
			SquareMatrix cp(*this);
			multiply(cp, M, *this);
			return *this;
		}

		SquareMatrix operator*(const SquareMatrix& M) const {
			SquareMatrix result = nil;
			multiply(*this, M, result);
			return result;
		}

		vector_type operator*(const vector_type& v) const {
			Vector<n, T> u;
			for(size_t i = 0; i < n; i++) {
				const T* row = (*this)[i];
				for(size_t j = 0; j < n; j++) {
					u[i] += row[j]*v[j];
				}
			}
			return u;
		}

		vector_type operator()(const vector_type& v) const {
			return (*this) * v;
		}
	};

	template<size_t n, typename T = float_type>
	class ProjectiveMatrix : public SquareMatrix<n + 1, T> {
		template<size_t M, typename U>
		friend class Vector;

		template<size_t M, typename U>
		friend class SquareMatrix;

		template<size_t M, typename U>
		friend class ProjectiveMatrix;

	protected:
		typedef SquareMatrix<n + 1, T> super;
		typedef typename super::super vectorsuper;

	public:
		// For matrix-vector multiplication representing translation.
		typedef Vector<n, T> affine_vector_type;
		typedef affine_vector_type projective_vector_type;

		ProjectiveMatrix() : super() {}
		ProjectiveMatrix(Nil) : super(nil) {}
		ProjectiveMatrix(T diag) : super(diag) {}
		ProjectiveMatrix(const ProjectiveMatrix& A) : super(A) {}
		explicit ProjectiveMatrix(const super& A) : super(A) {}

		ProjectiveMatrix& operator=(const super& A) {
			super::operator=(A);
			return *this;
		}


		ProjectiveMatrix& operator*=(const super& M) {
			super::operator*=(M);
			return *this;
		}

		ProjectiveMatrix operator*(const super& M) const {
			ProjectiveMatrix result = nil;
			multiply(*this, M, result);
			return result;
		}

		affine_vector_type operator*(const affine_vector_type& v) const {
			Vector<n, T> u;
			for(size_t i = 0; i < n; i++) {
				const T* row = (*this)[i];
				for(size_t j = 0; j < n; j++) {
					u[i] += row[j]*v[j];
				}
				u[i] += row[n];
			}
			return u;
		}

		affine_vector_type operator()(const affine_vector_type& v) const {
			return (*this) * v;
		}

		void getLinearPart(SquareMatrix<n, T>& M) const {
			for(size_t i = 0; i < n; i++) {
				const T* row = (*this)[i];
				T* Mrow = M[i];
				for(size_t j = 0; j < n; j++) {
					Mrow[j] = row[j];
				}
			}
		}

		SquareMatrix<n, T> getLinearPart() const {
			SquareMatrix<n, T> result = nil;
			getLinearPart(result);
			return result;
		}

		void setLinearPart(const SquareMatrix<n, T>& M) {
			for(size_t i = 0; i < n; i++) {
				T* row = (*this)[i];
				const T* Mrow = M[i];
				for(size_t j = 0; j < n; j++) {
					row[j] = Mrow[j];
				}
			}
		}

		void getTranslation(affine_vector_type& v) const {
			for(size_t i = 0; i < n; i++) {
				v[i] = (*this)[i][n];
			}
		}

		void setTranslation(affine_vector_type& v) {
			for(size_t i = 0; i < n; i++) {
				(*this)[i][n] = v[i];
			}
		}

		affine_vector_type getTranslation() const {
			Vector<n, T> result = nil;
			getTranslation(result);
			return result;
		}
	};

	/*
	 * Takes the coordinates of what is taken to be a 2x2 matrix.
	 */
	template<typename T>
	inline void raw_invert(T& a, T& b, T& c, T& d) {
		T invdet = 1/(a*d - c*b);

		std::swap( a, d );

		a *= invdet;
		d *= invdet;
		c *= -invdet;
		b *= -invdet;
	}

	template<typename T>
	inline void invert(SquareMatrix<2, T>& M) {
		raw_invert( M[0][0], M[0][1], M[1][0], M[1][1] );
	}

	template<typename T>
	inline SquareMatrix<2, T> inverseOf(const SquareMatrix<2, T>& M) {
		SquareMatrix<2, T> N(M);
		invert(N);
		return N;
	}

	template<typename T>
	inline void invert(ProjectiveMatrix<2, T>& M) {
		Vector<2, T> v;
		M.getTranslation(v);

		M[0][2] = 0;
		M[1][2] = 0;

		raw_invert(M[0][0], M[0][1], M[1][0], M[1][1]);

		M.setTranslation( -M*v );
	}

	template<typename T>
	inline void inverseOf(const ProjectiveMatrix<2, T>& M) {
		ProjectiveMatrix<2, T> N(M);
		invert(N);
		return N;
	}

	template<typename T = float_type, size_t N = 3>
	class Triangle {
	public:
		typedef Vector<N, T> vertex_type;
		typedef Vector<N, T> vector_type;
		typedef Vector<N, T> point_type;

		vector_type a, b, c;

		Triangle() : a(), b(), c() {}
		Triangle(const vertex_type& _a, const vertex_type& _b, const vertex_type& _c) : a(_a), b(_b), c(_c) {}
		Triangle(const Triangle& trig) : a(trig.a), b(trig.b), c(trig.c) {}

		Triangle& operator+=(const vector_type& v) {
			a += v;
			b += v;
			c += v;
			return *this;
		}

		Triangle operator+(const vector_type& v) const {
			return Triangle(*this) += v;
		}
	};

	template<size_t coord, typename bbox_t>
	class BoundingBoxAccessor {};

	template<typename bbox_t>
	class BoundingBoxAccessor<0, bbox_t> {
	public:
		typedef typename bbox_t::scalar_type scalar_type;

		static inline scalar_type min(const bbox_t& bbox) {
			return bbox.x();
		}

		static inline scalar_type max(const bbox_t& bbox) {
			return bbox.xmax();
		}
	};

	template<typename bbox_t>
	class BoundingBoxAccessor<1, bbox_t> {
	public:
		typedef typename bbox_t::scalar_type scalar_type;

		static inline scalar_type min(const bbox_t& bbox) {
			return bbox.y();
		}

		static inline scalar_type max(const bbox_t& bbox) {
			return bbox.ymax();
		}
	};

	template<typename T = float_type>
	class BoundingBox {
	public:
		typedef Vector<2, T> vector_type;
		typedef vector_type point_type;
		typedef T scalar_type;

	private:
		vector_type bottom_left;
		vector_type top_right;
		bool initialized;

		static int dim_to_int(float val) {
			return int(ceilf(val));
		}

		static int dim_to_int(double val) {
			return int(ceil(val));
		}

		static const size_t X_COORD = 0;
		static const size_t Y_COORD = 1;

		template<size_t primary_coord, size_t secondary_coord>
		static bool lex_compare(const BoundingBox& A, const BoundingBox& B) {
			typedef BoundingBoxAccessor<primary_coord, BoundingBox> major;
			typedef BoundingBoxAccessor<secondary_coord, BoundingBox> minor;

			const T major_A_min = major::min(A);
			const T major_B_min = major::min(B);

			if(major_A_min < major_B_min) {
				return true;
			}
			else if(major_A_min > major_B_min) {
				return false;
			}

			const T minor_A_min = minor::min(A);
			const T minor_B_min = minor::min(B);

			if(minor_A_min < minor_B_min) {
				return true;
			}
			else if(minor_A_min > minor_B_min) {
				return false;
			}

			/*
			 * The corners coincide.
			 * Now we just check if A is a proper subset of B.
			 */

			const float_type A_xmax = A.xmax();
			const float_type B_xmax = B.xmax();

			if(A_xmax > B_xmax) {
				return false;
			}
			if(A_xmax == B_xmax) {
				return A.ymax() < B.ymax();
			}
			else {
				return A.ymax() <= B.ymax();
			}
		}

	public:
		BoundingBox() : bottom_left(), top_right(), initialized(false) {}
		BoundingBox(T _x, T _y, T _w, T _h) {
			setDimensions(_x, _y, _w, _h);
		}

		BoundingBox(const BoundingBox& bbox) :
			bottom_left(bbox.bottom_left),
			top_right(bbox.top_right),
			initialized(bbox.initialized) {
		}

		operator bool() const {
			return initialized;
		}

		inline bool lexXYLess(const BoundingBox& bbox) const {
			return lex_compare<X_COORD, Y_COORD>(*this, bbox);
		}

		inline bool lexYXLess(const BoundingBox& bbox) const {
			return lex_compare<Y_COORD, X_COORD>(*this, bbox);
		}

		const vector_type& bottomLeft() const {
			return bottom_left;
		}

		const vector_type& topRight() const {
			return top_right;
		}

		T x() const {
			return bottom_left[0];
		}

		void x(T _x) {
			bottom_left[0] = _x;
		}

		T xmax() const {
			return top_right[0];
		}

		void xmax(T _x) {
			top_right[0] = _x;
		}

		T y() const {
			return bottom_left[1];
		}

		void y(T _y) {
			bottom_left[1] = _y;
		}

		T ymax() const {
			return top_right[1];
		}
		
		void ymax(T _y) {
			top_right[1] = _y;
		}

		T w() const {
			return top_right[0] - bottom_left[0];
		}

		void w(T _w) {
			top_right[0] = bottom_left[0] + _w;
		}

		T h() const {
			return top_right[1] - bottom_left[1];
		}

		void h(T _h) {
			top_right[1] = bottom_left[1] + _h;
		}

		int int_w() const {
			return dim_to_int(w());
		}

		int int_h() const {
			return dim_to_int(h());
		}

		void reset() {
			bottom_left[0] = 0;
			bottom_left[1] = 0;
			top_right[0] = 0;
			top_right[1] = 0;
			initialized = false;
		}

		void setDimensions(T _x, T _y, T _w, T _h) {
			bottom_left[0] = _x;
			bottom_left[1] = _y;
			top_right[0] = _x + _w;
			top_right[1] = _y + _h;
			initialized = true;
		}

		void intersect(const BoundingBox& bb) {
			if(x() < bb.x()) {
				x(bb.x());
			}
			if(y() < bb.y()) {
				y(bb.y());
			}
			if(xmax() > bb.xmax()) {
				xmax(bb.xmax());
			}
			if(ymax() > bb.ymax()) {
				ymax(bb.ymax());
			}
		}

		void addPoint(T u, T v) {
			if(!initialized) {
				bottom_left[0] = u;
				bottom_left[1] = v;
				top_right[0] = u;
				top_right[1] = v;
				initialized = true;
				return;
			}

			if(u < bottom_left[0]) {
				bottom_left[0] = u;
			}
			else if(u > top_right[0]) {
				top_right[0] = u;
			}

			if(v < bottom_left[1]) {
				bottom_left[1] = v;
			}
			else if(v > top_right[1]) {
				top_right[1] = v;
			}
		}

		template<size_t N>
		void addPoint(const Vector<N, T>& v) {
			staticAssert<N >= 2>();
			addPoint(v[0], v[1]);
		}

		template<size_t N>
		void addPoints(const Triangle<T, N>& t) {
			addPoint(t.a);
			addPoint(t.b);
			addPoint(t.c);
		}
	};
}

#endif
