import torch
import numpy as np
import pandas as pd

#DEVICE = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
DEVICE = torch.device("cpu")

def make_training_and_testing_set(data, percent_train=90.):
    num_train = int(float(percent_train) * data.shape[0] / 100)
    num_test = data.shape[0] - num_train

    _train_x = torch.from_numpy(data[num_test:, :-1])
    _train_y = torch.from_numpy(data[num_test:, 1:])
    _test_x = torch.from_numpy(data[:num_test, :-1])
    _test_y = torch.from_numpy(data[:num_test, 1:])
    return _train_x, _train_y, _test_x, _test_y


def min_max_scale(x):
    _min = np.min(x)
    _max = np.max(x)
    return (x - _min) / (_max - _min)


def one_hot_encode_column(df, column_to_encode, prefix):
    df[column_to_encode] = pd.Categorical(df[column_to_encode])
    df_dummies = pd.get_dummies(df[column_to_encode], prefix=prefix)
    df = pd.concat([df, df_dummies], axis=1)
    df = df.drop(columns=[column_to_encode])
    return df


def preprocess_data(df):
    # Drop task_code since it is not required.
    df = df.drop(columns=['task_code', 'pid'])

    # Get the time column
    time_np = np.diff(df['time'].values, n=1)
    time_np[time_np.argmax()] = 0  # Hack particular to this dataset.

    # Normalize the time column
    time_np_diff_scaled = min_max_scale(time_np)

    # Drop the last column to match sizes.
    df.drop(df.tail(1).index, inplace=True)
    df['time'] = time_np_diff_scaled.tolist()

    # One hot encoding of the task names.
    df = one_hot_encode_column(df=df, column_to_encode='name', prefix='name')
    return df
