using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Ragnarok;
using Ragnarok.Presentation;

namespace Bonako
{
    internal sealed class Settings : WPFSettingsBase
    {
        public string AS_Name
        {
            get { return GetValue<string>("AS_Name"); }
            set { SetValue("AS_Name", value); }
        }

        public int AS_ThreadNum
        {
            get { return GetValue<int>("AS_ThreadNum"); }
            set { SetValue("AS_ThreadNum", value); }
        }

        public int AS_HashMemSize
        {
            get { return GetValue<int>("AS_HashMemSize"); }
            set { SetValue("AS_HashMemSize", value); }
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        public Settings()
        {
        }
    }
}
