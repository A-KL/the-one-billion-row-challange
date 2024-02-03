#pragma once

struct city_info_avg_t {

    void add_temp(float temp) {

        _values += temp;
        _values_count++;

        if (temp > _max) {
            _max = temp;
        }
        if (temp < _min) {
            _min = temp;
        }
    }

    float get_mid() const {

        if (_values_count == 0) {
            return -1000;
        }
        return _values / _values_count;
    }

    inline int min_temp() const {
        return _min;
    }

    inline int max_temp() const {
        return _max;
    }

private:
    int _min = 1000;
    int _max = -1000;
    double _values = 0;
    long _values_count = 0;
};
