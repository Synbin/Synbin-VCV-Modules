struct ARGen
{
    int state;
    float output;
    float output_inc;
    float output_dec;
    const float MAX_VOLTS = 5.0f;

    enum envState
    {
        env_idle = 0,
        env_attack,
        env_release
    };

    void setAttackRate(float rate)
    {
        output_inc = MAX_VOLTS / rate;
    }

    void setReleaseRate(float rate)
    {
        output_dec = MAX_VOLTS / rate;
    }

    float process()
    {

        switch (state)
        {
        case env_idle:
            break;

        case env_attack:
            output += output_inc;
            if (output >= MAX_VOLTS)
            {
                output = MAX_VOLTS;
                state = env_release;
            }
            break;

        case env_release:
            output -= output_dec;
            if (output <= 0.0)
            {
                output = 0.0;
                state = env_idle;
            }
            break;
        }
        return output;
    }

    void reset()
    {
        state = env_idle;
        output = 0.0;
    }
};
