#ifndef KRANE_ALGEBRA_HPP
#define KRANE_ALGEBRA_HPP

#include "ktools_common.hpp"

namespace KTools {
	class ALGEBRA_METATYPE {};

	template<size_t N, typename T>
	class Vector;

	template<size_t n, typename T>
	class SquareMatrix;

	template<size_t n, typename T>
	class ProjectiveMatrix; 


	template<size_t N, typename T = float>
	class Vector {
		template<size_t M, typename U>
		friend class Vector;

		template<size_t M, typename U>
		friend class SquareMatrix;

		template<size_t M, typename U>
		friend class ProjectiveMatrix;

	protected:
		T x[N];

		// no init constructor.
		Vector(ALGEBRA_METATYPE _) {(void)_;}

	public:
		Vector() {
			for(size_t i = 0; i < N; i++) {
				x[i] = T();
			}
		}

		Vector(T pad) {
			for(size_t i = 0; i < N; i++) {
				x[i] = pad;
			}
		}

		Vector& operator=(const Vector& v) {
			for(size_t i = 0; i < N; i++) {
				x[i] = v.x[i];
			}
		}

		Vector(const Vector& v) {
			*this = v;
		}

		Vector copy() const {
			return Vector(*this);
		}

		static size_t dimension() {
			return N;
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
			Vector<M, T> v = ALGEBRA_METATYPE();
			for(size_t i = 0; i < (M < N ? M : N); i++) {
				v.x[i] = x[i];
			}
			for(size_t i = N; i < M; i++) {
				v.x[i] = pad;
			}
			return v;
		}
	};

	template<size_t n, typename T = float>
	class SquareMatrix : public Vector<n*n, T> {
		template<size_t M, typename U>
		friend class Vector;

		template<size_t M, typename U>
		friend class SquareMatrix;

		template<size_t M, typename U>
		friend class ProjectiveMatrix;

	protected:
		typedef Vector<n*n, T> super;

		SquareMatrix(ALGEBRA_METATYPE _) : super(_) {}

	public:
		SquareMatrix& setDiagonal(T diag = T(1)) {
			for(size_t i = 0; i < n; i++) {
				(*this)[i][i] = diag;
			}
		}

		SquareMatrix() : super() {
			setDiagonal(1);
		}
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
			SquareMatrix result = ALGEBRA_METATYPE();
			multiply(*this, M, result);
			return result;
		}

		Vector<n, T> operator*(const Vector<n, T>& v) const {
			Vector<n, T> u;
			for(size_t i = 0; i < n; i++) {
				const T* row = (*this)[i];
				for(size_t j = 0; j < n; j++) {
					u[i] += row[j]*v[j];
				}
			}
			return u;
		}

		Vector<n, T> operator()(const Vector<n, T>& v) const {
			return (*this) * v;
		}
	};

	template<size_t n, typename T = float>
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

		ProjectiveMatrix(ALGEBRA_METATYPE _) : super(_) {}

	public:
		ProjectiveMatrix() : super() {}
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
			ProjectiveMatrix result = ALGEBRA_METATYPE();
			multiply(*this, M, result);
			return result;
		}

		Vector<n, T> operator*(const Vector<n, T>& v) const {
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

		Vector<n, T> operator()(const Vector<n, T>& v) const {
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
			SquareMatrix<n, T> result = ALGEBRA_METATYPE();
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

		void getTranslation(Vector<n, T>& v) const {
			for(size_t i = 0; i < n; i++) {
				v[i] = (*this)[i][n];
			}
		}

		void setTranslation(const Vector<n, T>& v) {
			for(size_t i = 0; i < n; i++) {
				(*this)[i][n] = v[i];
			}
		}

		Vector<n, T> getTranslation() const {
			Vector<n, T> result = ALGEBRA_METATYPE();
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

	/*
	 * The first element has the xyz coordinates, the second the uvw ones.
	 */
	typedef std::pair< Vector<3>, Vector<3> > AtlasVertex;
}

#endif
