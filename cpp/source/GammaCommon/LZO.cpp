
#include "GammaCommon/LZO.h"

namespace Gamma
{
	unsigned _do_compress( const tbyte *in, unsigned in_len, tbyte *out, unsigned *out_len )
	{
		long wrkmem [16384L];
		const tbyte *ip;
		tbyte *op;
		const tbyte *in_end = in + in_len;
		const tbyte *ip_end = in + in_len -3;
		const tbyte *ii; // 指向开始编码的位置
		const tbyte **dict = (const tbyte **)wrkmem;
		op = out;
		ip = in;
		ii = ip;
		ip += 4;
		for(;;)
		{
			const tbyte *m_pos;
			unsigned m_off;
			unsigned m_len;
			unsigned dindex; // hashkey(ip[0], ip[1], ip[2], ip[3])
			dindex = ((0x21*(((((((unsigned)(ip[3])<<6)^ip[2])<<5)^ip[1])<<5)^ip[0]))>>5) & 0x3fff;
			m_pos = dict [dindex];
			if(((uintptr_t)m_pos < (uintptr_t)in) ||
				(m_off = (unsigned)((uintptr_t)ip-(uintptr_t)m_pos) ) <= 0 ||
				m_off > 0xbfff) // 0xc000 48kb
				goto literal;
			if(m_off <= 0x0800 || m_pos[3] == ip[3]) // 回指长度小于2Kb
				goto try_match;
			dindex = (dindex & 0x7ff ) ^ 0x201f; // 处理冲突,第二次hash
			m_pos = dict[dindex];
			if((uintptr_t)(m_pos) < (uintptr_t)(in) ||
				(m_off = (unsigned)(uintptr_t)( (int)((uintptr_t)ip-(uintptr_t)m_pos))) <= 0 ||
				m_off > 0xbfff)
				goto literal;
			if (m_off <= 0x0800 || m_pos[3] == ip[3]) // 回指长度小于2Kb
				goto try_match; // 第三个字节相等
			goto literal;
try_match: // m_pos[0],m_pos[1],m_pos[2]都匹配成功时,继续比较
			if(*(unsigned short*)m_pos == *(unsigned short*)ip && m_pos[2]== ip[2])
				goto match;
			literal: // 匹配不成功时,或者无记录
			dict[dindex] = ip; // 记录字符串为ip[0],ip[1],ip[2],ip[3]的地址
			++ip;
			if (ip >= ip_end)
				break;
			continue;
match: // 在得到匹配长度与位置之前,先输出未匹配的字符
			dict[dindex] = ip; // 更新，字符匹配时的位置(未编码)
			if(ip - ii > 0) // 存在新字符
			{
                unsigned t = (unsigned)( ip - ii ); // t:新字符的数目(未匹配的)
				if (t <= 3) // 新字符数目<3时
					op[-2] |= (tbyte)t; // 对两个保留字元赋值
				else if(t <= 18) // 新字符数目<18时
					*op++ = (tbyte)(t - 3);
				else
				{
					 unsigned tt = t - 18;
					*op++ = 0;
					while(tt > 255) // 构建新位元组
					{
						tt -= 255;
						*op++ = 0;
					}
					*op++ = (tbyte)tt;
				}
				do
				{
					*op++ = *ii++; // ii指向开始匹配的位置(未编码)
				}while (--t > 0); // 输出 t个新字符
			}
			ip += 3; // 跳过与m_pos[0] m_pos[1] m_pos[2]的比较
			if(m_pos[3] != *ip++ || m_pos[4] != *ip++ || m_pos[5] != *ip++ ||
				m_pos[6] != *ip++ || m_pos[7] != *ip++ || m_pos[8] != *ip++ )
			{
				--ip;
				m_len = (unsigned)( ip - ii ); // 得到重复长度<=8
				if(m_off <= 0x0800 ) // 回指长度小于2kb
				{
					--m_off; // m_off,与m_len在输出时都减1
					// m_off在第一位元组(tbyte)占三位,m_off&7 小于8
					*op++ = (tbyte)(((m_len - 1) << 5) | ((m_off & 7) << 2));
					*op++ = (tbyte)(m_off >> 3); // 去除已用的低3位
				}
				else
					if (m_off <= 0x4000 ) // 回指长度小于16kb
					{
						-- m_off;
						*op++ = (tbyte)(32 | (m_len - 2));
						goto m3_m4_offset;
					}
					else // 回指长度大于16时
					{
						m_off -= 0x4000;
						*op++ = (tbyte)(16 | ((m_off & 0x4000) >> 11) | (m_len - 2));
						goto m3_m4_offset;
					}
			}
			else // 重复长度大于8时
			{
				{
					const tbyte *end = in_end;
					const tbyte *m = m_pos + 9; // 从m_pos[9]开始比较
					while (ip < end && *m == *ip)
					{ m++; ip++; }
					m_len = (unsigned)(ip - ii);
				}
				if(m_off <= 0x4000) // 回指长度小于16kb
				{
					--m_off;
					if (m_len <= 33) // 可用5bit表示时
						*op++ = (tbyte)(32 | (m_len - 2));
					else
					{
						m_len -= 33;
						*op++ = 32;
						goto m3_m4_len;
					}
				}
				else // 回指长度大于16kb ,小于48 kb
				{
					m_off -= 0x4000;
					if(m_len <= 9)
						*op++ = (tbyte)(16|((m_off & 0x4000) >> 11) | (m_len - 2));
					else
					{
						m_len -= 9;
						*op++ = (tbyte)(16 | ((m_off & 0x4000) >> 11));
m3_m4_len:
						while (m_len > 255)
						{
							m_len -= 255;
							*op++ = 0;
						}
						*op++ = (tbyte)m_len;
					}
				}
m3_m4_offset:
				*op++ = (tbyte)((m_off & 63) << 2);
				*op++ = (tbyte)(m_off >> 6);
			}
			ii = ip; // 下次匹配的开始位置
			if (ip >= ip_end)
				break;
		}
		*out_len = (unsigned)(op - out);
		return (unsigned) (in_end - ii);
	}


	//C语言解压缩算法
	GAMMA_COMMON_API int32 lzo_compress(const tbyte *in, unsigned in_len, tbyte *out)
	{
		tbyte *op = out;
		unsigned t,out_len;
		if (in_len <= 13)
			t = in_len;
		else
		{
			t = _do_compress (in,in_len,op,&out_len);
			op += out_len;
		}

		if (t > 0) // t: 未编码的字符大小,即新字符的数目
		{
			tbyte *ii = (tbyte*)in + in_len - t; // 未编码的开始地址
			if (op == (tbyte*)out && t <= 238)
				*op++ = (tbyte) ( 17 + t );
			else
				if (t <= 3) // 新字符数目<3时
					op[-2] |= (tbyte)t ;
				else
					if (t <= 18) // 新字符数目<18 时
						*op++ = (tbyte)(t-3);
					else
					{
						unsigned tt = t - 18;
						*op++ = 0;
						while (tt > 255)
						{
							tt -= 255;
							*op++ = 0;
						}
						*op++ = (tbyte)tt;
					}
					do
					{
						*op++ = *ii++;
					}while (--t > 0); // 输出t个新字符
		}
		*op++ = 17; // 结束编码标志
		*op++ = 0;
		*op++ = 0;
		return (int32)(op - (tbyte*)out); // 返回编码后的长度
	}

	GAMMA_COMMON_API int32 lzo_decompress(const tbyte *in, unsigned in_len, tbyte *out)
	{
        tbyte *op; // 输出临时缓存区
        const tbyte *ip;
        unsigned t;
        tbyte *m_pos;
		tbyte *ip_end = (tbyte*)in + in_len;
		op = out;
		ip = in;
		if(*ip > 17)
		{
			t = *ip++ - 17;
			if (t < 4)
				goto match_next;
			do *op++ = *ip++; while (--t > 0);
			goto first_literal_run;
		}
		for(;;)
		{
			t = *ip++; // 得到新字符的数目(t+3)
			if (t >= 16) // 新字符数目(t+3) > 18时
				goto match;
			if (t == 0) // 新字符数目大于18时
			{
				while (*ip == 0)
				{
					t += 255;
					ip++;
				}
				t += 15 + *ip++; // 得到具体新字符数目大小(t+3)
			}
			// 获取t新字符,每次以4个为单位
			* (unsigned *) op = * ( unsigned *) ip; // 获取sizeof(unsigned)个新字符
			op += 4;
			ip += 4;
			if (--t > 0) // 新字符数目:t+4-1 = t + 3,已处理了4个
			{
				if (t >= 4)
				{
					do
					{
						// 获取sizeof(unsigned)个新字符,即4个,以4个为单位
						* (unsigned * ) op = * ( unsigned * ) ip;
						op += 4;
						ip += 4;
						t -= 4;
					} while (t >= 4);
					if (t > 0) // 不足一个单位时,且t>0
					{
						do
						{
							*op++ = *ip++;
						}while (--t > 0);
					}
				}
				else
				{
					do
					{
						*op++ = *ip++;
					}while (--t > 0);
				}
			}
first_literal_run:
			t = *ip++; // 判断是否是重复字符编码
			if (t >= 16) // 是重复字符编码
				goto match;
			m_pos = op - 0x0801;
			m_pos -= t >> 2;
			m_pos -= *ip++ << 2;
			*op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
			goto match_done;
			for(;;)
			{
match:
				// 根据第一单元组来判断其解压种类
				if (t >= 64) // 回指长度小于2Kb
				{
					// int match_len = (*ip++ << 3) | ((t >> 2) & 7) + 1;
					// m_pos= op -match_len; //得到匹配位置
					m_pos = op - 1; //
					m_pos -= (t >> 2) & 7; // 得到第一个位元组的distance
					m_pos -= *ip++ << 3; //得到匹配位置
					t = (t >> 5) - 1; // 得到第一个位元组的len - 1
					goto copy_match;
				}
				else if (t >= 32) // 回指长度大于2Kb < 16kb
				{
					t &= 31;
					if (t == 0)
					{
						while (*ip == 0)
						{
							t += 255;
							ip++;
						}
						t += 31 + *ip++;
					}
					m_pos = op - 1;
					m_pos -= (* ( unsigned short * ) ip) >> 2;
					ip += 2;
				}
				else if (t >= 16) // 回指长度大于16 Kb,或者结束标志
				{
					m_pos = op;
					m_pos -= (t & 8) << 11; // 获得第一个单元组的distance
					t &= 7; // 获取第一个单元组的len
					if (t == 0)
					{
						while (*ip == 0)
						{
							t += 255;
							ip++;
						}
						t += 7 + *ip++;
					}
					m_pos -= (* ( unsigned short *) ip) >> 2;
					ip += 2;
					if (m_pos == op) // 判断是否为结束标志
						goto eof_found;
					m_pos -= 0x4000;
				}
				else
				{
					m_pos = op - 1;
					m_pos -= t >> 2;
					m_pos -= *ip++ << 2;
					*op++ = *m_pos++; *op++ = *m_pos;
					goto match_done;
				}
				if (t >= 6 && (op - m_pos) >= 4)
				{
					* (unsigned *) op = * ( unsigned *) m_pos;
					op += 4;
					m_pos += 4;
					t -= 2;
					do
					{
						* (unsigned *) op = * ( unsigned *) m_pos;
						op += 4;
						m_pos += 4;
						t -= 4;
					}while (t >= 4);
					if (t > 0)
						do
						{
							*op++ = *m_pos++;
						}while (--t > 0);
				}
				else
				{
copy_match:
					*op++ = *m_pos++; // 获得前两个匹配字符
					*op++ = *m_pos++;
					do
					{
						*op++ = *m_pos++;
					}while (--t > 0); // 获得剩余的匹配字符
				}
match_done:
				t = ip[-2] & 3; // 获取保留位,当新字符数目<=3时
				if (t == 0) // 保留位未使用时,即新字符数目>3
					break;
match_next:
				do
				{
					*op++ = *ip++;
				}while (--t > 0);
				t = *ip++; // 下一个匹配单元
			}
		}
eof_found:
		if (ip != ip_end)
			return -1;
		return (int32)(op - (tbyte*)out); // 返回解码后的长度
	}
}
