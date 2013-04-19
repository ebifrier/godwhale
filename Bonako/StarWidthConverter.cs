using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Globalization;

using Ragnarok;

namespace Bonako
{
    /// <summary>
    /// リストビューのあるカラムの幅を全体の幅にフィットするようにします。
    /// </summary>
    [ValueConversion(typeof(ListView), typeof(double))]
    public class StarWidthConverter : IMultiValueConverter
    {
        public object Convert(object[] value, Type targetType,
                              object parameter, CultureInfo culture)
        {
            var listView = value[0] as ListView;
            if (listView == null)
            {
                return 0.0;
            }

            var gridView = listView.View as GridView;
            if (gridView == null)
            {
                return 0.0;
            }

            var listViewWidth = (double)value[1];
            var selfIndex = int.Parse(parameter.ToString());
            var colWidth = gridView.Columns
                .WhereWithIndex((_, i) => i != selfIndex)
                .Select(_ => _.Width)
                .Where(_ => !double.IsNaN(_))
                .Sum();

            // 5 is to take care of margin/padding
            return Math.Max(1.0, listViewWidth - colWidth - 5.0);
        }

        public object[] ConvertBack(object value, Type[] targetType,
                                    object parameter, CultureInfo culture)
        {
            return null;
        }
    }
}
