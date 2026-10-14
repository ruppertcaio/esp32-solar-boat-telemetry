#ifndef AVERAGE_H
#define AVERAGE_H

#include <cstddef>

// --------------------------------------------------------------
// Média móvel de janela fixa (buffer circular)
// T: tipo da amostra
// N: tamanho da janela (quantidade máxima de amostras)
// Sem alocação dinâmica; addSample() e value() são O(1).
// --------------------------------------------------------------
template<typename T, size_t N>
class Average {
    static_assert(N > 0, "A janela deve ter pelo menos 1 amostra");

public:

    // --------------------------------------------------------------
    // addSample()
    // Adiciona uma nova amostra ao filtro.
    // Com a janela cheia, sobrescreve a amostra mais antiga.
    // --------------------------------------------------------------
    void addSample(T sample) {
        if (count_ == N) {
            sum_ -= samples_[head_];   // remove a amostra mais antiga da soma
        } else {
            count_++;
        }

        samples_[head_] = sample;
        sum_ += sample;
        head_ = (head_ + 1) % N;

        // A cada volta completa recalcula a soma, para que o erro de
        // arredondamento (float) da soma corrente não se acumule.
        if (head_ == 0) {
            sum_ = 0;
            for (size_t i = 0; i < N; i++) sum_ += samples_[i];
        }
    }

    // --------------------------------------------------------------
    // value()
    // Retorna o valor médio das amostras armazenadas.
    // Se estiver vazio, retorna 0.
    // --------------------------------------------------------------
    T value() const {
        if (count_ == 0)
            return 0;

        return sum_ / static_cast<T>(count_);  // média
    }

private:
    T samples_[N] = {};   // buffer circular com as amostras
    T sum_ = 0;           // soma corrente das amostras na janela
    size_t head_ = 0;     // posição da próxima escrita
    size_t count_ = 0;    // amostras válidas (até N)
};

#endif
